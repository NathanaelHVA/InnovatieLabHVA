/*!
 * \file    main.c
 * \brief   Toetsen + ADC + serieel, gecombineerd met een golfgenerator op DAC B (PB2)
 *
 * Toets ingedrukt -> frequentie uit frequentie_centihz() gaat naar de golf.
 * Golfvorm kiezen: zie golf_init(), haal // weg bij de gewenste regel.
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

#define TICK_MS             10      // lusduur: toetsen worden elke 10 ms gepolld
#define ADC_PERIODE_TICKS   20      // ADC elke 20 ticks = 200 ms

#define FS 100000UL            

static uint16_t lut[256];           // 12-bit tabel van de gekozen golfvorm
static volatile uint32_t phase = 0;
static volatile uint32_t step  = 0;

ISR(TCC0_OVF_vect) {
    phase += step;
    DACB.CH0DATA = lut[phase >> 24];   
}

// frequentie in centi-Hz (zoals frequentie_centihz() levert)
static void setFreq_centihz(uint32_t f_centi) {
    uint32_t s = (uint32_t)(((uint64_t)f_centi << 32) / (FS * 100UL));
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { step = s; }
}

static void klok_32mhz(void) {
    OSC.CTRL |= OSC_RC32MEN_bm;
    while (!(OSC.STATUS & OSC_RC32MRDY_bm));
    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
}

static void golf_init(void) {
    for (int i = 0; i < 256; i++) {
        lut[i] = (uint16_t)(2047 + 2047 * sin(2 * M_PI * i / 256.0));       // sinus
        //lut[i] = (i < 128) ? 4095 : 0;                                    // blok
        //lut[i] = (uint16_t)((uint32_t)i * 4095UL / 255UL);                // zaagtand
        //lut[i] = (uint16_t)((uint32_t)((i < 128) ? i : 255 - i) * 4095UL / 127UL);  // driehoek
    }

    // DAC B, kanaal 0 (PB2)
    DACB.CTRLB = DAC_CHSEL_SINGLE_gc;
    DACB.CTRLC = DAC_REFSEL_AVCC_gc;
    DACB.CTRLA = DAC_CH0EN_bm | DAC_ENABLE_bm;

    // Timer TCC0 als sampleklok (100 kHz)
    TCC0.PER = F_CPU / FS - 1;          // 319
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
    golf_init();
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

        // --- ADC, elke 200 ms ---
        if (++ticks >= ADC_PERIODE_TICKS) {
            ticks = 0;

            adc_meting_t m = adc_meet();
            printf("res:%5d  procent:%3d%%  procent2:%3d%%\n",
                   m.res, m.procent, m.procent2);
        }

        _delay_ms(TICK_MS);
    }
}
