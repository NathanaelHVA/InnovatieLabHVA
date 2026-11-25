/*!
 * \file    main.c
 * \brief   Signaalgenerator op een ESP32-S3: 2 VCO's naar 2 DAC80501's, frequentie en volume intypen
 *
 * Zo werkt het:
 *   - Core 1 doet NIETS anders dan samples maken (vco_taak). Hij wacht op de cycle counter van de
 *     CPU tot het volgende sample-moment, stuurt het klaarliggende sample naar beide DAC's en
 *     rekent dan het volgende uit:  fase += stap  ->  waarde uit de golftabel  ->  x volume
 *   - Core 0 (app_main) doet elke 10 ms de rest: terminal lezen, volume laten lopen, gate.
 *
 * Wat je kunt instellen:
 *   - samplefrequentie       : FS
 *   - golfvorm per VCO       : VCO1_GOLF en VCO2_GOLF
 *   - toonhoogte van VCO 2   : VCO2_RATIO_Q8
 *   - frequentie             : intypen (440 of 261.63 + Enter), of START_FREQ_CENTIHZ
 *   - volume                 : intypen (v0 .. v100 + Enter), of VOLUME_START_PROCENT
 *
 * Pinnen: DAC's zie dac80501.h (GPIO21, 45, 47, 48), gate GPIO4, gate geïnverteerd GPIO5
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "sdkconfig.h"

#include "dac80501.h"
#include "freq_input.h"

/* ---------- instellingen ---------- */

// Samplefrequentie. 96 kHz -> Nyquist 48 kHz, dus je 20 kHz-filter past ruim.
#define FS                    96000UL

// Golfvorm per VCO: GOLF_SINUS, GOLF_BLOK, GOLF_ZAAGTAND of GOLF_DRIEHOEK
#define VCO1_GOLF             GOLF_SINUS
#define VCO2_GOLF             GOLF_ZAAGTAND

// Toonhoogte van VCO 2 t.o.v. VCO 1 in 1/256: 256 = gelijk, 512 = een octaaf hoger, 257 = lichte detune
#define VCO2_RATIO_Q8         256UL

// Frequentie bij het opstarten, in centi-Hz (100000 = 1000,00 Hz)
#define START_FREQ_CENTIHZ    100000UL

// Volume bij het opstarten (0..100 %). Het volume loopt vanaf 0 hiernaartoe: zachte start, geen plop.
#define VOLUME_START_PROCENT  100UL

// Hoe snel het volume naar een nieuwe waarde loopt: zoveel stapjes (van 256) per tik van 10 ms.
// 8 -> van 0 naar vol in ~0,3 s, zo hoor je geen klik als je een nieuw volume typt.
#define VOLUME_STAP           8

/* ---------- gate voor de envelope generator ---------- */

#define GATE_PIN              4           // gate
#define GATE_INV_PIN          5           // gate geïnverteerd
#define GATE_PERIODE_MS       6000UL      // hele periode
#define GATE_HOOG_MS          3000UL      // zo lang is de gate hoog (3000 = 50% duty)
#define TIK_MS                10UL        // core 0 doet elke 10 ms zijn rondje

_Static_assert(GATE_HOOG_MS < GATE_PERIODE_MS, "GATE_HOOG_MS moet kleiner zijn dan de periode");
_Static_assert(GATE_HOOG_MS % TIK_MS == 0 && GATE_PERIODE_MS % TIK_MS == 0,
               "GATE-tijden moeten een veelvoud van TIK_MS zijn");

/* ---------- afgeleide waarden (niet aanpassen) ---------- */

// De sample-taak telt CPU-klokcycli. De echte samplefrequentie is CPU_HZ / FS_PERIODE
// (240 MHz / 2500 = precies 96 kHz; bij 160 MHz wordt het 96038 Hz).
#define CPU_HZ        ((uint32_t)CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * 1000000UL)
#define FS_PERIODE    (CPU_HZ / FS)                // klokcycli per sample
#define FS_ECHT       (CPU_HZ / FS_PERIODE)        // echte samplefrequentie in Hz

/* ---------- golfvormen en VCO's ---------- */

typedef enum { GOLF_SINUS, GOLF_BLOK, GOLF_ZAAGTAND, GOLF_DRIEHOEK, GOLF_AANTAL } golf_t;

static const char *golf_naam[GOLF_AANTAL] = { "sinus", "blok", "zaagtand", "driehoek" };

static uint16_t golf_tabel[GOLF_AANTAL][256];       // 16-bit tabellen voor de DAC80501

// Gedeeld tussen core 0 (schrijft) en core 1 (leest). 32-bit waarden en pointers worden
// op de ESP32 in één keer gelezen/geschreven, dus hier is geen lock nodig.
static volatile uint32_t          vco_step[2];
static const uint16_t * volatile  vco_lut[2];
static volatile uint32_t          volume;          // 0..256, 256 = vol (core 0 laat het naar volume_doel lopen)
static uint32_t                   volume_doel;

// Statistiek van de sample-taak (core 1 schrijft, core 0 leest)
static volatile uint32_t          vco_max_cycli;   // langste rekentijd per sample
static volatile uint32_t          vco_te_laat;     // aantal keer dat een sample te laat was

// Tabelwaarde bij een 32-bit fase: bovenste 8 bit = tabelindex, de 8 bit daaronder = fractie
static inline uint16_t lut_waarde(const uint16_t *lut, uint32_t phase)
{
    uint32_t i  = phase >> 24;
    int32_t  fr = (int32_t)((phase >> 16) & 0xFF);
    int32_t  a  = lut[i];
    int32_t  b  = lut[(i + 1) & 0xFF];             // na punt 255 weer naar punt 0
    return (uint16_t)(a + (((b - a) * fr) >> 8));
}

// Sample schalen rond het midden (32768), zodat de DC-waarde gelijk blijft bij elk volume
static inline uint16_t met_volume(uint16_t s, uint32_t vol)
{
    int32_t x = (int32_t)s - 32768;
    return (uint16_t)(32768 + ((x * (int32_t)vol) >> 8));
}

// De sample-taak: draait voor altijd op core 1 en geeft de core nooit af
static void IRAM_ATTR vco_taak(void *arg)
{
    (void)arg;
    dac80501_init();                               // op deze core: de GPIO-bundel hoort bij core 1

    uint32_t fase[2] = { 0, 0 };
    uint16_t s1 = 0x8000, s2 = 0x8000;             // klaarliggende samples (begin: stilte)
    uint32_t t = esp_cpu_get_cycle_count();

    for (;;) {
        // Wachten tot het volgende sample-moment
        t += FS_PERIODE;
        uint32_t nu;
        while ((int32_t)((nu = esp_cpu_get_cycle_count()) - t) < 0) { }
        if (nu - t > FS_PERIODE) {                 // meer dan een hele periode te laat: opnieuw beginnen
            t = nu;
            vco_te_laat++;
        }

        // Eerst versturen (dan zit er zo min mogelijk jitter op het moment), daarna het volgende uitrekenen
        dac80501_write2(s1, s2);

        uint32_t vol = volume;
        fase[0] += vco_step[0];
        fase[1] += vco_step[1];
        s1 = met_volume(lut_waarde(vco_lut[0], fase[0]), vol);
        s2 = met_volume(lut_waarde(vco_lut[1], fase[1]), vol);

        uint32_t gebruikt = esp_cpu_get_cycle_count() - nu;
        if (gebruikt > vco_max_cycli) vco_max_cycli = gebruikt;
    }
}

// frequentie in centi-Hz (440 Hz = 44000); VCO 2 volgt VCO 1 met VCO2_RATIO_Q8
// stap = f / FS * 2^32  (de fase loopt van 0 tot 2^32 = één periode)
static void setFreq_centihz(uint32_t f_centi)
{
    uint32_t s1 = (uint32_t)(((uint64_t)f_centi << 32) / ((uint64_t)FS_ECHT * 100ULL));
    uint32_t s2 = (uint32_t)(((uint64_t)s1 * VCO2_RATIO_Q8) >> 8);
    vco_step[0] = s1;
    vco_step[1] = s2;
}

// golfvorm van een VCO wisselen (vco = 0 of 1)
static void vco_set_golf(uint8_t vco, golf_t golf)
{
    vco_lut[vco] = golf_tabel[golf];
}

// volume in procent (0..100) -> 0..256
static void setVolume_procent(uint32_t p)
{
    volume_doel = (p * 256UL + 50UL) / 100UL;
}

// Volume stapje voor stapje naar volume_doel (elke TIK_MS)
static void volume_lopen(void)
{
    uint32_t v = volume, d = volume_doel;
    if (v < d)      volume = (d - v > VOLUME_STAP) ? v + VOLUME_STAP : d;
    else if (v > d) volume = (v - d > VOLUME_STAP) ? v - VOLUME_STAP : d;
}

// Wat er in de terminal getypt is meteen toepassen: "440" + Enter of "v50" + Enter
static void serieel_invoer(void)
{
    uint32_t w;
    switch (freq_input_poll(&w)) {
    case FREQ_INPUT_FREQ:
        setFreq_centihz(w);
        printf("\nFrequentie: %lu.%02lu Hz\n", (unsigned long)(w / 100), (unsigned long)(w % 100));
        break;
    case FREQ_INPUT_VOLUME:
        setVolume_procent(w);
        printf("\nVolume: %lu %%\n", (unsigned long)w);
        break;
    default:
        break;
    }
}

// Gate: blokgolf op GATE_PIN, precies omgekeerd op GATE_INV_PIN (elke TIK_MS aanroepen)
static void gate_init(void)
{
    gpio_set_level(GATE_PIN, 1);                   // start: gate hoog
    gpio_set_level(GATE_INV_PIN, 0);               //        inv laag
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << GATE_PIN) | (1ULL << GATE_INV_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
}

static void gate_tik(void)
{
    static uint32_t ms = 0;
    ms += TIK_MS;
    if (ms == GATE_HOOG_MS) {
        gpio_set_level(GATE_PIN, 0);               // gate omlaag, inv omhoog
        gpio_set_level(GATE_INV_PIN, 1);
    } else if (ms >= GATE_PERIODE_MS) {
        ms = 0;
        gpio_set_level(GATE_PIN, 1);               // gate omhoog, inv omlaag
        gpio_set_level(GATE_INV_PIN, 0);
    }
}

static void golf_init(void)
{
    for (int i = 0; i < 256; i++) {
        golf_tabel[GOLF_SINUS][i]    = (uint16_t)(32767.5 + 32767.0 * sin(2 * M_PI * i / 256.0));
        golf_tabel[GOLF_BLOK][i]     = (i < 128) ? 65535 : 0;
        golf_tabel[GOLF_ZAAGTAND][i] = (uint16_t)((uint32_t)i * 65535UL / 255UL);
        golf_tabel[GOLF_DRIEHOEK][i] = (uint16_t)((uint32_t)((i < 128) ? i : 255 - i) * 65535UL / 127UL);
    }
    vco_set_golf(0, VCO1_GOLF);
    vco_set_golf(1, VCO2_GOLF);
}

/* ---------- main ---------- */

void app_main(void)
{
    setvbuf(stdin, NULL, _IONBF, 0);               // tekens direct doorgeven, niet per regel bufferen

    golf_init();
    setFreq_centihz(START_FREQ_CENTIHZ);
    setVolume_procent(VOLUME_START_PROCENT);
    gate_init();

    // Sample-taak op core 1, net onder de hoogste prioriteit (de IPC-taak van ESP-IDF mag nog voor)
    xTaskCreatePinnedToCore(vco_taak, "vco", 4096, NULL, configMAX_PRIORITIES - 2, NULL, 1);

    printf("\nSignaalgenerator ESP32-S3, %lu Hz samplefrequentie, CPU %d MHz\n",
           (unsigned long)FS_ECHT, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    printf("DAC 1: %s   DAC 2: %s\n", golf_naam[VCO1_GOLF], golf_naam[VCO2_GOLF]);
    printf("Typ een frequentie (440 of 261.63) of een volume (v0 .. v100) + Enter\n");

    TickType_t laatst = xTaskGetTickCount();
    uint32_t tikken = 0, te_laat_gemeld = 0;

    for (;;) {
        vTaskDelayUntil(&laatst, pdMS_TO_TICKS(TIK_MS));
        serieel_invoer();
        volume_lopen();
        gate_tik();

        // Na 1 s één keer laten zien hoe zwaar de sample-taak is
        if (++tikken == 1000 / TIK_MS) {
            printf("Sample-taak: max %lu van %lu cycli per sample (%lu %%)\n",
                   (unsigned long)vco_max_cycli, (unsigned long)FS_PERIODE,
                   (unsigned long)(vco_max_cycli * 100UL / FS_PERIODE));
        }
        if (vco_te_laat != te_laat_gemeld) {
            te_laat_gemeld = vco_te_laat;
            printf("Let op: sample-taak %lu keer te laat, FS te hoog?\n", (unsigned long)te_laat_gemeld);
        }
    }
}
