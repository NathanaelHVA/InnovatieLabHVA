/*!
 * \file    moving_discs.c
 * \author  Caspar Treijtel (c.treijtel@hva.nl)
 * \brief   Simple demo library that draws discs on the TFT display, which
 *          is used with the ucglib library. The discs can be created with
 *          different colors and radius. The discs can be moved on the
 *          display.
 * \version 0.1
 * \date    2023-10-02
 */
#include "moving_discs.h"
#include <stdlib.h>
#include "serialF0.h"

color_t *md_create_color(uint8_t r, uint8_t g, uint8_t b)
{
    color_t *c = (color_t *) malloc(sizeof(color_t));
    if ( c != NULL ) {
        c->red = r;
        c->green = g;
        c->blue = b;
    }

    return c;
}

void md_set_color(color_t *c, uint8_t r, uint8_t g, uint8_t b)
{
    if ( c != NULL ) {
        c->red = r;
        c->green = g;
        c->blue = b;
    }
}

disc_t *md_create_disc(ucg_t *ucg, uint8_t nr, ucg_int_t rad, color_t *c)
{
    disc_t *d = (disc_t *) malloc(sizeof(disc_t));
    if ( d != NULL ) {
        d->ucg = ucg;
        d->nr = nr;
        d->rad = rad;
        d->color = c;
    }

    return d;
}

void md_init_disc(disc_t *disc, ucg_t *ucg, uint8_t nr, ucg_int_t rad,
               color_t *c) {

    if ( disc != NULL ) {
        disc->ucg = ucg;
        disc->nr = nr;
        disc->rad = rad;
        disc->color = c;
    }
}

void md_set_disc_position(disc_t *disc, ucg_int_t x, ucg_int_t y)
{
    if ( disc != NULL ) {
        disc->x = x;
        disc->y = y;
    }
}

void md_move_disc(disc_t *disc, ucg_int_t dx, ucg_int_t dy)
{
    ucg_int_t xcur, ycur;
    // Draw black circle
    xcur = disc->x;
    ycur = disc->y;
    ucg_SetColor(disc->ucg, 0, 0, 0, 0);
    ucg_DrawDisc(disc->ucg, xcur, ycur, disc->rad, UCG_DRAW_ALL);

    // Move new circle
    disc->x += dx;
    disc->y += dy;
    if (disc->x > (X_LINES + disc->rad/2)) {
        disc->x = 0;
    } else if (disc->x < 0) {
        disc->x = X_LINES + disc->rad/2;
    }
    if (disc->y > (Y_LINES + disc->rad/2)) {
        disc->y = 0;
    } else if (disc->y < 0) {
        disc->y = Y_LINES + disc->rad/2;
    }
    // Draw new circle
    ucg_SetColor(disc->ucg, 0,
                 disc->color->red,
                 disc->color->green,
                 disc->color->blue);
    ucg_DrawDisc(disc->ucg, disc->x, disc->y, disc->rad, UCG_DRAW_ALL);
}

void md_print_disc_position(disc_t* d)
{
    printf("Postion disc %d=(%d,%d) ", d->nr, d->x, d->y);
}
