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

#include <avr/io.h>
#include <util/delay.h>
#include "clock.h"
#include "ucglib_xmega.h"
#include "vco_ui.h"

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

    while (1) {
        /* Vanuit jullie eigen code aanroepen als er iets verandert:
         *   ui_set_octaaf(&ucg, -2 .. 2);
         *   ui_set_golf(&ucg, 1 of 2, GOLF_SINUS / GOLF_BLOK / GOLF_DRIEHOEK / GOLF_ZAAGTAND);
         *   ui_set_volume(&ucg, 1 of 2, 0 .. 100);
         * Alleen wat verandert wordt opnieuw getekend (en groen bij selectie).
         */


         ui_set_octaaf(&ucg, -2);              // -2 .. 2
ui_set_golf(&ucg, 1, GOLF_BLOK);     // VCO 1 of 2
ui_set_golf(&ucg, 2, GOLF_SINUS);    // GOLF_SINUS / GOLF_BLOK / GOLF_DRIEHOEK / GOLF_ZAAGTAND
        ui_set_volume(&ucg, 1, 50);          // 0 ..
ui_set_volume(&ucg, 2, 75);          // 0 .. 100 %
        _delay_ms(20);
    }
}
