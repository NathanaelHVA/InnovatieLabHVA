#define F_CPU 2000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>

#include "serialF0.h"
#include "adc.h"
#include "toetsen.h"
#include "frequentie.h"

#define TICK_MS             10      // lusduur: toetsen worden elke 10 ms gepolld
#define ADC_PERIODE_TICKS   20      // ADC elke 20 ticks = 200 ms

int main(void)
{
    uint8_t ticks = 0;

    init_stream(F_CPU);
    adc_init();
    toetsen_init();
    sei();

    printf("Systeem gestart\n");

    while (1) {
        // --- Toetsen en octaaf ---
        toetsen_event_t ev = toetsen_poll();

        if (ev.octaaf) {
            printf("Octaaf: %u\n", toetsen_octaaf());
        }

        for (uint8_t i = 0; i < AANTAL_TOETSEN; i++) {
            if (ev.toetsen & (1U << i)) {
                uint32_t f = frequentie_centihz(i, toetsen_octaaf());

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