/*!
 * \file    vco_ui.c
 * \brief   VCO-scherm voor het ST7735 display (128x160, staand).
 */
#include "vco_ui.h"
#include <stdio.h>

/* ---------- Kleuren ---------- */
#define WIT     255, 255, 255
#define ZWART     0,   0,   0
#define GROEN     0, 170,  80

/* ---------- Layout (pixels) ---------- */
#define BREEDTE        128
#define HOOGTE         160

#define LIJN_TITEL      18      /* lijn onder "VCO"           */
#define LIJN_OCTAAF     51      /* lijn onder octaaf-blok     */
#define LIJN_VCO1      104      /* lijn tussen VCO 1 en VCO 2 */

#define OCT_Y           43      /* baseline octaafcijfers     */
#define OCT_X0          28      /* midden van "-2"            */
#define OCT_STAP        18      /* afstand tussen cijfers     */

#define KNOP_R          11      /* straal golfvorm-knop       */
#define KNOP_X0         20      /* midden eerste knop         */
#define KNOP_STAP       29      /* afstand tussen knoppen     */

/* y-posities per VCO (index 0 = VCO 1, index 1 = VCO 2) */
static const uint8_t vco_titel_y[2]  = {  65, 118 };
static const uint8_t vco_volume_y[2] = {  78, 131 };
static const uint8_t vco_knop_y[2]   = {  91, 145 };

/* ---------- Huidige toestand (om alleen veranderingen te tekenen) ---------- */
static int8_t  huidig_octaaf = 0;
static golf_t  huidig_golf[2]   = { GOLF_SINUS, GOLF_SINUS };
static uint8_t huidig_volume[2] = { 0, 0 };

/* ---------- Hulpfuncties ---------- */

static void tekst_midden(ucg_t *ucg, ucg_int_t cx, ucg_int_t y, const char *s)
{
    ucg_int_t w = ucg_GetStrWidth(ucg, s);
    ucg_DrawString(ucg, cx - w / 2, y, 0, s);
}

/* Lijn van 2 pixels dik */
static void dikke_lijn(ucg_t *ucg, ucg_int_t x1, ucg_int_t y1,
                       ucg_int_t x2, ucg_int_t y2)
{
    ucg_DrawLine(ucg, x1, y1, x2, y2);
    ucg_DrawLine(ucg, x1 + 1, y1, x2 + 1, y2);
}

/* Tekent een golfvorm-symbool rond (cx, cy), ca. 14 x 8 pixels */
static void teken_golf(ucg_t *ucg, ucg_int_t cx, ucg_int_t cy, golf_t golf)
{
    /* sinus: 1 periode, 13 punten, amplitude 4 */
    static const int8_t sinus[13] = { 0, -2, -3, -4, -3, -2, 0, 2, 3, 4, 3, 2, 0 };
    uint8_t i;

    switch (golf) {
    case GOLF_SINUS:
        for (i = 0; i < 12; i++) {
            dikke_lijn(ucg, cx - 7 + i, cy + sinus[i],
                            cx - 6 + i, cy + sinus[i + 1]);
        }
        break;

    case GOLF_BLOK:
        dikke_lijn(ucg, cx - 7, cy + 4, cx - 4, cy + 4);
        dikke_lijn(ucg, cx - 4, cy + 4, cx - 4, cy - 4);
        dikke_lijn(ucg, cx - 4, cy - 4, cx + 3, cy - 4);
        dikke_lijn(ucg, cx + 3, cy - 4, cx + 3, cy + 4);
        dikke_lijn(ucg, cx + 3, cy + 4, cx + 6, cy + 4);
        break;

    case GOLF_DRIEHOEK:
        dikke_lijn(ucg, cx - 7, cy + 4, cx - 4, cy - 4);
        dikke_lijn(ucg, cx - 4, cy - 4, cx,     cy + 4);
        dikke_lijn(ucg, cx,     cy + 4, cx + 3, cy - 4);
        dikke_lijn(ucg, cx + 3, cy - 4, cx + 6, cy + 4);
        break;

    case GOLF_ZAAGTAND:
        dikke_lijn(ucg, cx - 7, cy + 4, cx - 1, cy - 4);
        dikke_lijn(ucg, cx - 1, cy - 4, cx - 1, cy + 4);
        dikke_lijn(ucg, cx - 1, cy + 4, cx + 5, cy - 4);
        dikke_lijn(ucg, cx + 5, cy - 4, cx + 5, cy + 4);
        break;

    default:
        break;
    }
}

/* Tekent 1 knop: groen gevuld als geselecteerd, anders wit met zwarte rand */
static void teken_knop(ucg_t *ucg, uint8_t vco_idx, golf_t golf, uint8_t geselecteerd)
{
    ucg_int_t cx = KNOP_X0 + golf * KNOP_STAP;
    ucg_int_t cy = vco_knop_y[vco_idx];

    if (geselecteerd) {
        ucg_SetColor(ucg, 0, GROEN);
        ucg_DrawDisc(ucg, cx, cy, KNOP_R, UCG_DRAW_ALL);
        ucg_SetColor(ucg, 0, ZWART);
        ucg_DrawCircle(ucg, cx, cy, KNOP_R, UCG_DRAW_ALL);
        ucg_SetColor(ucg, 0, WIT);
    } else {
        ucg_SetColor(ucg, 0, WIT);
        ucg_DrawDisc(ucg, cx, cy, KNOP_R, UCG_DRAW_ALL);
        ucg_SetColor(ucg, 0, ZWART);
        ucg_DrawCircle(ucg, cx, cy, KNOP_R, UCG_DRAW_ALL);
        ucg_DrawCircle(ucg, cx, cy, KNOP_R - 1, UCG_DRAW_ALL);
    }
    teken_golf(ucg, cx, cy, golf);
}

/* Tekent 1 octaafcijfer (-2 .. 2) in zwart of groen */
static void teken_octaaf_cijfer(ucg_t *ucg, int8_t oct, uint8_t geselecteerd)
{
    char buf[6];
    ucg_int_t cx = OCT_X0 + (oct + 2) * OCT_STAP;

    /* oude cijfer wegpoetsen */
    ucg_SetColor(ucg, 0, WIT);
    ucg_DrawBox(ucg, cx - 7, OCT_Y - 10, 15, 12);

    snprintf(buf, sizeof(buf), "%d", oct);
    if (geselecteerd) {
        ucg_SetColor(ucg, 0, GROEN);
    } else {
        ucg_SetColor(ucg, 0, ZWART);
    }
    ucg_SetFont(ucg, ucg_font_helvB08_tr);
    tekst_midden(ucg, cx, OCT_Y, buf);
}

static void teken_volume(ucg_t *ucg, uint8_t vco_idx, uint8_t procent)
{
    char buf[16];
    ucg_int_t y = vco_volume_y[vco_idx];

    /* oude tekst wegpoetsen */
    ucg_SetColor(ucg, 0, WIT);
    ucg_DrawBox(ucg, 14, y - 10, 100, 12);

    snprintf(buf, sizeof(buf), "Volume: %u%%", procent);
    ucg_SetColor(ucg, 0, ZWART);
    ucg_SetFont(ucg, ucg_font_helvB08_tr);
    tekst_midden(ucg, BREEDTE / 2, y, buf);
}

/* ---------- Publieke functies ---------- */

void ui_init(ucg_t *ucg)
{
    uint8_t v;
    int8_t  o;
    golf_t  g;

    ucg_SetFontMode(ucg, UCG_FONT_MODE_TRANSPARENT);

    /* witte achtergrond */
    ucg_SetColor(ucg, 0, WIT);
    ucg_DrawBox(ucg, 0, 0, BREEDTE, HOOGTE);

    /* kader en scheidingslijnen (2 px dik) */
    ucg_SetColor(ucg, 0, ZWART);
    ucg_DrawFrame(ucg, 0, 0, BREEDTE, HOOGTE);
    ucg_DrawFrame(ucg, 1, 1, BREEDTE - 2, HOOGTE - 2);
    ucg_DrawBox(ucg, 0, LIJN_TITEL,  BREEDTE, 2);
    ucg_DrawBox(ucg, 0, LIJN_OCTAAF, BREEDTE, 2);
    ucg_DrawBox(ucg, 0, LIJN_VCO1,   BREEDTE, 2);

    /* titel */
    ucg_SetFont(ucg, ucg_font_helvB12_tr);
    tekst_midden(ucg, BREEDTE / 2, 15, "VCO");

    /* octaaf */
    ucg_SetFont(ucg, ucg_font_helvB08_tr);
    tekst_midden(ucg, BREEDTE / 2, 30, "Octaaf");
    for (o = -2; o <= 2; o++) {
        teken_octaaf_cijfer(ucg, o, o == huidig_octaaf);
    }

    /* VCO 1 en VCO 2 */
    for (v = 0; v < 2; v++) {
        ucg_SetColor(ucg, 0, ZWART);
        ucg_SetFont(ucg, ucg_font_helvB10_tr);
        tekst_midden(ucg, BREEDTE / 2, vco_titel_y[v], v == 0 ? "VCO 1" : "VCO 2");

        teken_volume(ucg, v, huidig_volume[v]);

        for (g = 0; g < GOLF_AANTAL; g++) {
            teken_knop(ucg, v, g, g == huidig_golf[v]);
        }
    }
}

void ui_set_octaaf(ucg_t *ucg, int8_t octaaf)
{
    if (octaaf < -2 || octaaf > 2 || octaaf == huidig_octaaf) {
        return;
    }
    teken_octaaf_cijfer(ucg, huidig_octaaf, 0);   /* oude terug naar zwart */
    teken_octaaf_cijfer(ucg, octaaf, 1);          /* nieuwe groen          */
    huidig_octaaf = octaaf;
}

void ui_set_golf(ucg_t *ucg, uint8_t vco, golf_t golf)
{
    uint8_t v = vco - 1;

    if (vco < 1 || vco > 2 || golf >= GOLF_AANTAL || golf == huidig_golf[v]) {
        return;
    }
    teken_knop(ucg, v, huidig_golf[v], 0);
    teken_knop(ucg, v, golf, 1);
    huidig_golf[v] = golf;
}

void ui_set_volume(ucg_t *ucg, uint8_t vco, uint8_t procent)
{
    uint8_t v = vco - 1;

    if (vco < 1 || vco > 2) {
        return;
    }
    if (procent > 100) {
        procent = 100;
    }
    if (procent == huidig_volume[v]) {
        return;
    }
    teken_volume(ucg, v, procent);
    huidig_volume[v] = procent;
}
