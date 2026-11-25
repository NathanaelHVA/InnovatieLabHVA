/*!
 * \file    main.c
 * \brief   VCO-scherm op het ST7735 display (128x160, staand) + 12 toetsen, octaafknop en
 *          volumepotmeter, op een ESP32-S3 met ESP-IDF.
 *
 *          Alle pinnen staan in pinnen.h.
 */
#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ucglib_esp32.h"
#include "vco_ui.h"
#include "adc.h"
#include "toetsen.h"
#include "frequentie.h"

#define TICK_MS            10   // lus elke 10 ms (debounce toetsen)
#define ADC_PERIODE_TICKS  20   // 20 x 10 ms = ADC elke 200 ms

void app_main(void)
{
    static ucg_t ucg;          // static: niet op de (kleine) stack van app_main

    ucg_Init(&ucg, ucg_dev_st7735_18x128x160, ucg_ext_st7735_18, ucg_com_esp32_cb);
    ucg_ClearScreen(&ucg);

    /* Hele scherm tekenen */
    ui_init(&ucg);

    /* Beginwaarden (zoals in het ontwerp) */
    static const char *golf_naam[GOLF_AANTAL] = { "sinus", "blok", "driehoek", "zaagtand" };
    golf_t golf[2] = { GOLF_DRIEHOEK, GOLF_DRIEHOEK };
    ui_set_octaaf(&ucg, 0);
    ui_set_golf(&ucg, 1, golf[0]);
    ui_set_golf(&ucg, 2, golf[1]);
    ui_set_volume(&ucg, 1, 0);
    ui_set_volume(&ucg, 2, 0);

    adc_init();
    toetsen_init();

    printf("Systeem gestart\n");

    uint8_t ticks = 0;
    TickType_t laatst = xTaskGetTickCount();

    while (1)
    {
        // --- Toetsen en octaaf ---
        toetsen_event_t ev = toetsen_poll();

        if (ev.octaaf)
        {
            printf("Octaaf: %u\n", toetsen_octaaf());
            ui_set_octaaf(&ucg, toetsen_octaaf()); // 0 .. 4
        }

        // --- Golfvormknoppen: sinus -> blok -> driehoek -> zaagtand -> sinus ... ---
        for (uint8_t v = 0; v < 2; v++)
        {
            if (ev.golf & (1U << v))
            {
                golf[v] = (golf_t)((golf[v] + 1) % GOLF_AANTAL);
                ui_set_golf(&ucg, v + 1, golf[v]);
                printf("VCO %u: %s\n", v + 1, golf_naam[golf[v]]);
            }
        }

        if (ev.reserve)
        {
            printf("Reserveknop (GPIO5) ingedrukt\n");
        }

        for (uint8_t i = 0; i < AANTAL_TOETSEN; i++)
        {
            if (ev.toetsen & (1U << i))
            {
                uint32_t f = frequentie_centihz(i, toetsen_octaaf());

                printf("Toets %u (GPIO%d): %lu.%02lu Hz\n",
                       i, toetsen_gpio(i),
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

        vTaskDelayUntil(&laatst, pdMS_TO_TICKS(TICK_MS));   // was _delay_ms(TICK_MS)
    }
}
