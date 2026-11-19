/*!
 * \file    moving_discs.h
 * \author  Caspar Treijtel (c.treijtel@hva.nl)
 * \brief   Simple demo library that draws discs on the TFT display, which
 *          is used with the ucglib library. The discs can be created with
 *          different colors and radius. The discs can be moved on the
 *          display.
 * \version 0.1
 * \date    2023-10-02
 */
#ifndef MOVING_DISCS_H
#define MOVING_DISCS_H

#include "ucglib/csrc/ucg.h"
#include "ucglib_xmega.h"

#define X_LINES 160
#define Y_LINES 128


typedef struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color_t;

typedef struct disc {
    ucg_t *ucg;
    ucg_int_t nr;
    ucg_int_t x;
    ucg_int_t y;
    ucg_int_t rad;
    color_t *color;
} disc_t;


color_t *md_create_color(uint8_t r, uint8_t g, uint8_t b);
void md_set_color(color_t *c, uint8_t r, uint8_t g, uint8_t b);
disc_t *md_create_disc(ucg_t *ucg, uint8_t nr, ucg_int_t rad, color_t *c);
void md_init_disc(disc_t *disc, ucg_t *ucg, uint8_t nr, ucg_int_t rad,
               color_t *c);
void md_set_disc_position(disc_t *disc, ucg_int_t x, ucg_int_t y);
void md_move_disc(disc_t *disc, ucg_int_t, ucg_int_t);
void md_print_disc_position(disc_t *disc);

#endif /* MOVING_DISCS_H */
