/*!
 * \file    main.c
 * \brief   VCO-scherm op het ST7735 display (128x160, staand).
 *
 *          Aansluiting (hardware SPI op SPID):
 *            VCC -> 3V3     GND -> GND
 *            CS  -> D4      RESET -> D3     A0 -> D2
 *            SDA -> D5      SCK   -> D7     LED -> 3V3
 */
#define F_CPU 32000000UL

#define TICK_MS            10   // lus elke 10 ms (debounce toetsen)
#define ADC_PERIODE_TICKS  20   // 20 x 10 ms = ADC elke 200 ms

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "clock.h"
#include "ucglib_xmega.h"
#include "vco_ui.h"
#include "serialF0.h"
#include "adc.h"
#include "toetsen.h"
#include "frequentie.h"

int main(void)
{
    

    init_clock();

    ucg_t ucg;
    ucg_Init(&ucg, ucg_dev_st7735_18x128x160, ucg_ext_st7735_18, ucg_com_xmega_cb);
    ucg_ClearScreen(&ucg);

    /* Hele scherm tekenen */
    ui_init(&ucg);

    /* Beginwaarden (zoals in het ontwerp) */
    ui_set_octaaf(&ucg, 0);
    ui_set_golf(&ucg, 1, GOLF_DRIEHOEK);
    ui_set_golf(&ucg, 2, GOLF_DRIEHOEK);
    ui_set_volume(&ucg, 1, 0);
    ui_set_volume(&ucg, 2, 0);

    uint8_t ticks = 0;

    init_stream(F_CPU);
    adc_init();
    toetsen_init();
    sei();

    printf("Systeem gestart\n");

    while (1)
    {
        // --- Toetsen en octaaf ---
        toetsen_event_t ev = toetsen_poll();

        if (ev.octaaf)
        {
            printf("Octaaf: %u\n", toetsen_octaaf());
            ui_set_octaaf(&ucg, toetsen_octaaf()); // 0 .. 4
        }

        for (uint8_t i = 0; i < AANTAL_TOETSEN; i++)
        {
            if (ev.toetsen & (1U << i))
            {
                uint32_t f = frequentie_centihz(i, toetsen_octaaf());

                printf("Toets %u (P%c%u): %lu.%02lu Hz\n",
                       i, toetsen_poort(i), toetsen_pin(i),
                       (unsigned long)(f / 100),
                       (unsigned long)(f % 100));
            }
        }

        // --- ADC, elke 200 ms ---
        if (++ticks >= ADC_PERIODE_TICKS)
        {
            ticks = 0;

            adc_meting_t m = adc_meet();
            printf("res:%5d  procent:%3d%%  procent2:%3d%%\n",
                   m.res, m.procent, m.procent2);
                   ui_set_volume(&ucg, 1, m.procent);
                   ui_set_volume(&ucg, 2, m.procent2);
        }

        _delay_ms(TICK_MS);
    }
}
/* Vanuit jullie eigen code aanroepen als er iets verandert:
 *   ui_set_octaaf(&ucg, -2 .. 2);
 *   ui_set_golf(&ucg, 1 of 2, GOLF_SINUS / GOLF_BLOK / GOLF_DRIEHOEK / GOLF_ZAAGTAND);
 *   ui_set_volume(&ucg, 1 of 2, 0 .. 100);
 * Alleen wat verandert wordt opnieuw getekend (en groen bij selectie).
 */

//         ui_set_octaaf(&ucg, -2);              // -2 .. 2
// ui_set_golf(&ucg, 1, GOLF_BLOK);     // VCO 1 of 2
// ui_set_golf(&ucg, 2, GOLF_SINUS);    // GOLF_SINUS / GOLF_BLOK / GOLF_DRIEHOEK / GOLF_ZAAGTAND
//        ui_set_volume(&ucg, 1, 50);          // 0 ..
// ui_set_volume(&ucg, 2, 75);          // 0 .. 100 %
//        _delay_ms(20);
//    }
//}
