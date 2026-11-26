/*!
 * \file    main.c
 * \brief   VCO-synth op een ESP32-S3: 2 VCO's naar 2 DAC80501's, met scherm, toetsen en knoppen
 *
 * Taakverdeling:
 *   - Core 1 doet NIETS anders dan samples maken (vco_taak). Hij wacht op de cycle counter van de
 *     CPU tot het volgende sample-moment, stuurt het klaarliggende sample naar beide DAC's en
 *     rekent dan het volgende uit:  fase += stap  ->  waarde uit de golftabel  ->  x volume
 *   - Core 0 (app_main) doet elke 10 ms de rest: toetsen, knoppen, potmeter, scherm, terminal, gate.
 *
 * Bediening:
 *   - toets 0..11           : toonhoogte (f = 130,8 Hz x 2^octaaf x 2^(toets/12)), gate hoog zolang ingedrukt
 *   - octaafknop            : octaaf 0 -> 1 -> 2 -> 3 -> 4 -> 0 ...
 *   - golfvormknop VCO 1/2  : sinus -> blok -> driehoek -> zaagtand -> sinus ...
 *   - potmeter              : volume verdelen, VCO 1 = x %, VCO 2 = 100 - x %
 *   - terminal              : frequentie (440 of 261.63 + Enter) en hoofdvolume (v0 .. v100 + Enter)
 *                             j + Enter: Vader Jakob aan/uit, p + Enter: He's a Pirate aan/uit (zie liedje.c);
 *                             een toets indrukken stopt het liedje ook
 *
 * Alle pinnen staan in pinnen.h.
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

#include "pinnen.h"
#include "dac80501.h"
#include "freq_input.h"
#include "ucglib_esp32.h"
#include "vco_ui.h"            // ook golf_t: GOLF_SINUS, GOLF_BLOK, GOLF_DRIEHOEK, GOLF_ZAAGTAND
#include "adc.h"
#include "toetsen.h"
#include "frequentie.h"
#include "liedje.h"

/* ---------- instellingen ---------- */

// Samplefrequentie. 96 kHz -> Nyquist 48 kHz, dus je 20 kHz-filter past ruim.
#define FS                    96000UL

// Liedje dat meteen na het opstarten begint: LIEDJE_VADER_JAKOB of LIEDJE_PIRAAT,
// of LIEDJE_GEEN = pas na j of p + Enter in de terminal
#define LIEDJE_BIJ_OPSTARTEN  LIEDJE_VADER_JAKOB

// Golfvorm bij het opstarten (zoals in het schermontwerp)
#define VCO1_START_GOLF       GOLF_DRIEHOEK
#define VCO2_START_GOLF       GOLF_DRIEHOEK

// Toonhoogte van VCO 2 t.o.v. VCO 1 in 1/256: 256 = gelijk, 512 = een octaaf hoger, 257 = lichte detune
#define VCO2_RATIO_Q8         256UL

// Hoofdvolume bij het opstarten (0..100 %), aan te passen met v0 .. v100 in de terminal
#define HOOFDVOLUME_START     50UL

// Hoe snel het volume naar een nieuwe waarde loopt: zoveel stapjes (van 256) per tik van 10 ms.
// 8 -> van 0 naar vol in ~0,3 s, zo hoor je geen klik bij een nieuw volume.
#define VOLUME_STAP           8

// 1 = gate hoog zolang er een toets is ingedrukt (zoals een keyboard)
// 0 = gate maakt zelf een blokgolf (GATE_PERIODE_MS / GATE_HOOG_MS), handig om te testen
#define GATE_VAN_TOETSEN      1
#define GATE_PERIODE_MS       6000UL
#define GATE_HOOG_MS          3000UL

#define TIK_MS                10UL        // core 0 doet elke 10 ms zijn rondje
#define ADC_PERIODE_TICKS     20          // potmeter elke 20 x 10 ms = 200 ms

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

static const char *golf_naam[GOLF_AANTAL] = {
    [GOLF_SINUS] = "sinus", [GOLF_BLOK] = "blok", [GOLF_DRIEHOEK] = "driehoek", [GOLF_ZAAGTAND] = "zaagtand",
};

static uint16_t golf_tabel[GOLF_AANTAL][256];       // 16-bit tabellen voor de DAC80501

// Gedeeld tussen core 0 (schrijft) en core 1 (leest). 32-bit waarden en pointers worden
// op de ESP32 in één keer gelezen/geschreven, dus hier is geen lock nodig.
static volatile uint32_t          vco_step[2];
static const uint16_t * volatile  vco_lut[2];
static volatile uint32_t          volume[2];        // per VCO 0..256, 256 = vol (loopt naar volume_doel)

// Statistiek van de sample-taak (core 1 schrijft, core 0 leest)
static volatile uint32_t          vco_max_cycli;   // langste rekentijd per sample
static volatile uint32_t          vco_te_laat;     // aantal keer dat een sample te laat was

// Alleen core 0
static golf_t   vco_golf[2];
static uint32_t volume_doel[2];
static uint32_t hoofdvolume = HOOFDVOLUME_START;    // 0..100 %, uit de terminal
static uint32_t pot_procent = 50;                   // 0..100 %, van de potmeter

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

        fase[0] += vco_step[0];
        fase[1] += vco_step[1];
        s1 = met_volume(lut_waarde(vco_lut[0], fase[0]), volume[0]);
        s2 = met_volume(lut_waarde(vco_lut[1], fase[1]), volume[1]);

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

// golfvorm van een VCO wisselen (vco = 0 of 1): geluid én scherm
static void vco_set_golf(ucg_t *ucg, uint8_t vco, golf_t golf)
{
    vco_golf[vco] = golf;
    vco_lut[vco]  = golf_tabel[golf];               // pointer: in één keer geschreven, core 1 ziet het direct
    ui_set_golf(ucg, vco + 1, golf);
}

// Doelvolume per VCO = hoofdvolume x verdeling van de potmeter
static void volume_herberekenen(void)
{
    uint32_t p[2] = { pot_procent, 100 - pot_procent };
    for (uint8_t v = 0; v < 2; v++)
        volume_doel[v] = (hoofdvolume * p[v] * 256UL + 5000UL) / 10000UL;   // % x % -> 0..256//* p[v]
}

// Volume stapje voor stapje naar volume_doel (elke TIK_MS)
static void volume_lopen(void)
{
    for (uint8_t i = 0; i < 2; i++) {
        uint32_t v = volume[i], d = volume_doel[i];
        if (v < d)      volume[i] = (d - v > VOLUME_STAP) ? v + VOLUME_STAP : d;
        else if (v > d) volume[i] = (v - d > VOLUME_STAP) ? v - VOLUME_STAP : d;
    }
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
        hoofdvolume = w;
      volume_herberekenen();
        printf("\nHoofdvolume: %lu %%\n", (unsigned long)w);
        break;
    case FREQ_INPUT_LIEDJE: {
        liedje_nr_t nr = (w == 'p') ? LIEDJE_PIRAAT : LIEDJE_VADER_JAKOB;
        if (liedje_bezig() && liedje_huidig() == nr) {     // hetzelfde liedje speelt al: stoppen
            liedje_stop();
            printf("\n%s gestopt\n", liedje_naam());
        } else {                                           // anders beginnen (ook als het andere liedje speelt)
            liedje_start(nr);
            printf("\n%s!\n", liedje_naam());
        }
        break;
    }
    default:
        break;
    }
}

/* ---------- gate ---------- */

// Gate en omgekeerde gate altijd samen zetten: GATE_INV_PIN is steeds het tegenovergestelde van GATE_PIN
static void gate_zet(int hoog)
{
    gpio_set_level(GATE_PIN,     hoog ? 1 : 0);
    gpio_set_level(GATE_INV_PIN, hoog ? 0 : 1);
}

static void gate_init(void)
{
    gate_zet(GATE_VAN_TOETSEN ? 0 : 1);                   // toetsen: begin laag; blokgolf: begin hoog
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
#if GATE_VAN_TOETSEN
    if (liedje_bezig()) gate_zet(liedje_gate());           // liedje speelt: gate volgt de noten
    else                gate_zet(toetsen_vast() ? 1 : 0);  // hoog zolang er een toets is ingedrukt
#else
    static uint32_t ms = 0;
    ms += TIK_MS;
    if (ms == GATE_HOOG_MS) {
        gate_zet(0);
    } else if (ms >= GATE_PERIODE_MS) {
        ms = 0;
        gate_zet(1);
    }
#endif
}

/* ---------- toetsen en knoppen ---------- */

static void knoppen_verwerken(ucg_t *ucg)
{
    toetsen_event_t ev = toetsen_poll();

    if (ev.octaaf) {
        printf("Octaaf: %u\n", toetsen_octaaf());
        ui_set_octaaf(ucg, toetsen_octaaf());               // 0 .. 4
    }

    for (uint8_t v = 0; v < 2; v++) {
        if (ev.golf & (1U << v)) {
            golf_t nieuw = (golf_t)((vco_golf[v] + 1) % GOLF_AANTAL);   // sinus -> blok -> driehoek -> zaagtand
            vco_set_golf(ucg, v, nieuw);
            printf("VCO %u: %s\n", v + 1, golf_naam[nieuw]);
        }
    }

    // Zelf spelen stopt het liedje
    if (ev.toetsen && liedje_bezig()) {
        liedje_stop();
        printf("%s gestopt\n", liedje_naam());
    }

    // Nieuwe toets: die bepaalt de toonhoogte (bij meerdere tegelijk wint de laatste in de lijst)
    for (uint8_t i = 0; i < AANTAL_TOETSEN; i++) {
        if (ev.toetsen & (1U << i)) {
            uint32_t f = frequentie_centihz(i, toetsen_octaaf());
            setFreq_centihz(f);
            printf("Toets %u (GPIO%d): %lu.%02lu Hz\n", i, toetsen_gpio(i),
                   (unsigned long)(f / 100), (unsigned long)(f % 100));
        }
    }
}


static void potmeter_lezen(ucg_t *ucg)
{
    adc_meting_t m = adc_meet();                    // procent al afgerond op 10-tallen
    if ((uint32_t)m.procent != pot_procent) {
        pot_procent = (uint32_t)m.procent;
        volume_herberekenen();
        printf("Potmeter: VCO 1 %d %%, VCO 2 %d %%\n", m.procent, m.procent2);
    }
    ui_set_volume(ucg, 1, (uint8_t)m.procent);      // tekent alleen opnieuw als het veranderd is
    ui_set_volume(ucg, 2, (uint8_t)m.procent2);
}


static void golf_init(void)
{
    for (int i = 0; i < 256; i++) {
        golf_tabel[GOLF_SINUS][i]    = (uint16_t)(32767.5 + 32767.0 * sin(2 * M_PI * i / 256.0));
        golf_tabel[GOLF_BLOK][i]     = (i < 128) ? 65535 : 0;
        golf_tabel[GOLF_ZAAGTAND][i] = (uint16_t)((uint32_t)i * 65535UL / 255UL);
        golf_tabel[GOLF_DRIEHOEK][i] = (uint16_t)((uint32_t)((i < 128) ? i : 255 - i) * 65535UL / 127UL);
    }
}

/* ---------- main ---------- */

void app_main(void)
{
    static ucg_t ucg;                               // static: niet op de stack van app_main

    setvbuf(stdin, NULL, _IONBF, 0);                // tekens direct doorgeven, niet per regel bufferen

    // Geluid klaarzetten en de sample-taak starten (volume begint op 0 en loopt rustig op)
    golf_init();
    vco_golf[0] = VCO1_START_GOLF;  vco_lut[0] = golf_tabel[VCO1_START_GOLF];
    vco_golf[1] = VCO2_START_GOLF;  vco_lut[1] = golf_tabel[VCO2_START_GOLF];
    setFreq_centihz(frequentie_centihz(0, 0));      // toets 0, octaaf 0
    gate_init();
    xTaskCreatePinnedToCore(vco_taak, "vco", 4096, NULL, configMAX_PRIORITIES - 2, NULL, 1);

    // Scherm, knoppen en potmeter
    ucg_Init(&ucg, ucg_dev_st7735_18x128x160, ucg_ext_st7735_18, ucg_com_esp32_cb);
    ucg_ClearScreen(&ucg);
    ui_init(&ucg);
    ui_set_octaaf(&ucg, 0);
    ui_set_golf(&ucg, 1, vco_golf[0]);
    ui_set_golf(&ucg, 2, vco_golf[1]);
    adc_init();
    toetsen_init();
    potmeter_lezen(&ucg);                           // begint meteen op de stand van de potmeter


    printf("\nVCO-synth ESP32-S3, %lu Hz samplefrequentie, CPU %d MHz\n",
           (unsigned long)FS_ECHT, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    printf("VCO 1: %s   VCO 2: %s\n", golf_naam[vco_golf[0]], golf_naam[vco_golf[1]]);
    printf("Typ een frequentie (440 of 261.63) of hoofdvolume (v0 .. v100) + Enter\n");
    printf("Typ j + Enter: Vader Jakob aan/uit, p + Enter: He's a Pirate aan/uit\n");

    liedje_start(LIEDJE_BIJ_OPSTARTEN);             // bij LIEDJE_GEEN gebeurt er niks
    if (liedje_bezig()) printf("%s!\n", liedje_naam());

    TickType_t laatst = xTaskGetTickCount();
    uint32_t tikken = 0, te_laat_gemeld = 0;

    for (;;) {
        vTaskDelayUntil(&laatst, pdMS_TO_TICKS(TIK_MS));

        serieel_invoer();
        knoppen_verwerken(&ucg);
        uint32_t f_lied;
        if (liedje_tik(TIK_MS, &f_lied)) setFreq_centihz(f_lied);   // nieuwe noot van het liedje
        gate_tik();
        if (tikken % ADC_PERIODE_TICKS == 0) potmeter_lezen(&ucg);
        volume_lopen();

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
