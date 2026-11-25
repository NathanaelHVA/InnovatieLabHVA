/*!
 * \file    vco_ui.h
 * \brief   VCO-scherm voor het ST7735 display (128x160, staand).
 *
 *          Layout:  titel "VCO"
 *                   Octaaf  -2 -1 0 1 2   (geselecteerd = groen)
 *                   VCO 1   Volume: xx%   4 golfvorm-knoppen
 *                   VCO 2   Volume: xx%   4 golfvorm-knoppen
 *
 *          Alle ui_set_... functies tekenen alleen opnieuw wat verandert.
 */
#ifndef VCO_UI_H
#define VCO_UI_H

#include <stdint.h>
#include "ucg.h"                 /* ucglib (components/ucglib) */

typedef enum {
    GOLF_SINUS = 0,
    GOLF_BLOK,
    GOLF_DRIEHOEK,
    GOLF_ZAAGTAND,
    GOLF_AANTAL
} golf_t;

/*! Tekent het hele scherm (1x aanroepen na ucg_Init). */
void ui_init(ucg_t *ucg);

/*! Octaaf -2 .. 2, de gekozen waarde wordt groen. */
void ui_set_octaaf(ucg_t *ucg, int8_t octaaf);

/*! vco = 1 of 2. De gekozen golfvorm krijgt een groene knop. */
void ui_set_golf(ucg_t *ucg, uint8_t vco, golf_t golf);

/*! vco = 1 of 2, volume 0 .. 100 %. */
void ui_set_volume(ucg_t *ucg, uint8_t vco, uint8_t procent);

#endif /* VCO_UI_H */
