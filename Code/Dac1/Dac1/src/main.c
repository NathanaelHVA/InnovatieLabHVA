/*!
 * \file    main.c
 * \brief   Toetsen + ADC + serieel, met VCO('s) op één of twee externe DAC80501 (SPI)
 *          en een gate voor de envelope generator
 *
 * Toets ingedrukt -> frequentie uit frequentie_centihz() gaat naar beide VCO's.
 *   VCO 1 -> DAC 1 (SYNC = PD4),  VCO 2 -> DAC 2 (SYNC = PF4),  SCLK = PD7, SDIN = PD5
 *   Nu staat DAC_AANTAL (dac80501.h) op 1: alleen VCO 1 -> DAC 1. VCO 2 / DAC 2 doet dan niet mee.
 * Gate voor de envelope generator (zie envelope.h): hoog zolang er minstens een toets ingedrukt is.
 * Golfvorm kiezen: zie VCO1_GOLF en VCO2_GOLF.
 * Frequentie intypen: typ in de seriële terminal bijvoorbeeld 440 of 261.63 + Enter (zie freq_input.h).
 */

#define F_CPU 32000000UL               // vóór de includes, één keer (klok gaat naar 32 MHz)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "serialF0.h"
#include "adc.h"
#include "toetsen.h"
#include "frequentie.h"
#include "dac80501.h"
#include "envelope.h"
#include "freq_input.h"

#define TICK_MS             10      // lusduur: toetsen worden elke 10 ms gepolld
#define ADC_PERIODE_TICKS   15      // ADC elke 20 ticks = 200 ms

// Samplefrequentie van de VCO's. Per sample gaan er twee SPI-frames van 24 bit over de bus
// (3 us) en de ISR zelf kost ongeveer 9 us (geschat uit de assembly, niet gemeten).
// Bij 100 kHz is de CPU dan bijna vol (10 us per sample); 50 kHz laat de hoofdlus ruim genoeg tijd.
// (Dat geldt voor 2 DAC's. Bij DAC_AANTAL 1 gaat er één frame over de bus en wordt maar één VCO berekend.)
#define FS 90000UL

// 1 = lineair interpoleren tussen twee tabelpunten (veel schoner signaal, kost per DAC ongeveer 1-2 us
//     extra in de ISR, niet gemeten); 0 = gewoon het dichtstbijzijnde tabelpunt (blokkerig, ~37 dB SNR)
#define VCO_INTERPOLATIE 1

// Golfvorm per VCO: GOLF_SINUS, GOLF_BLOK, GOLF_ZAAGTAND of GOLF_DRIEHOEK
#define VCO1_GOLF       GOLF_SINUS
#define VCO2_GOLF       GOLF_ZAAGTAND

// Toonhoogte van VCO 2 t.o.v. VCO 1 in 1/256: 256 = gelijk, 512 = een octaaf hoger, 257 = lichte detune
#define VCO2_RATIO_Q8   256UL

typedef enum { GOLF_SINUS, GOLF_BLOK, GOLF_ZAAGTAND, GOLF_DRIEHOEK, GOLF_AANTAL } golf_t;

static uint16_t golf_tabel[GOLF_AANTAL][256];   // 16-bit tabellen voor de DAC80501

static uint32_t                   vco_phase[2];  // alleen de ISR schrijft hierin
static volatile uint32_t          vco_step[2];
static const uint16_t * volatile  vco_lut[2];

// Tabelwaarde bij een 32-bit fase: bovenste 8 bit = tabelindex, de 8 bit daaronder = fractie
static inline uint16_t lut_waarde(const uint16_t *lut, uint32_t phase) {
#if VCO_INTERPOLATIE
    uint8_t  i  = (uint8_t)(phase >> 24);
    uint8_t  fr = (uint8_t)(phase >> 16);
    uint16_t a  = lut[i];
    uint16_t b  = lut[(uint8_t)(i + 1)];            // na punt 255 weer naar punt 0
    int32_t  d  = (int32_t)b - (int32_t)a;
    return (uint16_t)(a + ((d * fr) >> 8));
#else
    return lut[phase >> 24];
#endif
}

ISR(TCC0_OVF_vect) {
    vco_phase[0] += vco_step[0];
    dac80501_write(DAC_EXT1, lut_waarde(vco_lut[0], vco_phase[0]));
#if DAC_AANTAL > 1
    vco_phase[1] += vco_step[1];
    dac80501_write(DAC_EXT2, lut_waarde(vco_lut[1], vco_phase[1]));
#endif
}

// frequentie in centi-Hz (zoals frequentie_centihz() levert); VCO 2 volgt VCO 1 met VCO2_RATIO_Q8
static void setFreq_centihz(uint32_t f_centi) {
    uint32_t s1 = (uint32_t)(((uint64_t)f_centi << 32) / (FS * 100UL));
    uint32_t s2 = (uint32_t)(((uint64_t)s1 * VCO2_RATIO_Q8) >> 8);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { vco_step[0] = s1; vco_step[1] = s2; }
}

// golfvorm van een VCO wisselen (vco = 0 of 1)
static void vco_set_golf(uint8_t vco, golf_t golf) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { vco_lut[vco] = golf_tabel[golf]; }
}

// Een via de seriële poort ingetypte frequentie (bijvoorbeeld "440" + Enter) meteen toepassen
static void serieel_frequentie(void) {
    uint32_t f;
    if (freq_input_poll(&f)) {
        setFreq_centihz(f);
        printf("\nFrequentie: %lu.%02lu Hz\n",
               (unsigned long)(f / 100),
               (unsigned long)(f % 100));
    }
}

static void klok_32mhz(void) {
    OSC.CTRL |= OSC_RC32MEN_bm;
    while (!(OSC.STATUS & OSC_RC32MRDY_bm));
    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
}

static void golf_init(void) {
    for (int i = 0; i < 256; i++) {
        golf_tabel[GOLF_SINUS][i]    = (uint16_t)(32767.5 + 32767.0 * sin(2 * M_PI * i / 256.0));
        golf_tabel[GOLF_BLOK][i]     = (i < 128) ? 65535 : 0;
        golf_tabel[GOLF_ZAAGTAND][i] = (uint16_t)((uint32_t)i * 65535UL / 255UL);
        golf_tabel[GOLF_DRIEHOEK][i] = (uint16_t)((uint32_t)((i < 128) ? i : 255 - i) * 65535UL / 127UL);
    }
    vco_set_golf(0, VCO1_GOLF);
    vco_set_golf(1, VCO2_GOLF);

    // Timer TCC0 als sampleklok (100 kHz)
    TCC0.PER = F_CPU / FS - 1;          // 639 bij 50 kHz
    TCC0.INTCTRLA = TC_OVFINTLVL_HI_gc;
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
    PMIC.CTRL |= PMIC_HILVLEN_bm;
}

/* ---------- main ---------- */

int main(void)
{
    uint8_t ticks = 0;

    klok_32mhz();
    init_stream(F_CPU);
    adc_init();
    toetsen_init();
    envelope_init();           // gate-pinnen: gate laag, !gate hoog
    dac80501_init();           // SPID + de DAC('s) instellen (DAC_AANTAL)
    golf_init();               // tabellen + sampleklok (de ISR schrijft naar de DAC's)
    setFreq_centihz(100000UL);
    sei();


    while (1) {

        toetsen_event_t ev = toetsen_poll();

        if (ev.octaaf) {
            printf("Octaaf: %u\n", toetsen_octaaf());
        }

        for (uint8_t i = 0; i < AANTAL_TOETSEN; i++) {
            if (ev.toetsen & (1U << i)) {
                uint32_t f = frequentie_centihz(i, toetsen_octaaf());

                setFreq_centihz(f);

                printf("Toets %u (P%c%u): %lu.%02lu Hz\n",
                       i, toetsen_poort(i), toetsen_pin(i),
                       (unsigned long)(f / 100),
                       (unsigned long)(f % 100));
            }
        }

        // Gate voor de envelope: hoog zolang er een toets ingedrukt is.
        // Na de frequentie, zodat de envelope op de nieuwe toon start.
        envelope_gate(toetsen_ingedrukt() != 0);

        // --- ADC, elke 200 ms ---
        if (++ticks >= ADC_PERIODE_TICKS) {
            ticks = 0;

            adc_meting_t m = adc_meet();
            printf("res:%5d  procent:%3d%%  procent2:%3d%%\n",
                   m.res, m.procent, m.procent2);
            freq_input_debug();
        }

        // TICK_MS wachten, maar elke ms kijken of er een frequentie is ingetypt
        for (uint8_t t = 0; t < TICK_MS; t++) {
            _delay_ms(1);
            serieel_frequentie();
        }
    }
}