/*!
 * \file    freq_input.c
 * \brief   Frequentie intypen via de seriële terminal (zie freq_input.h)
 */

#include <stdio.h>
#include "freq_input.h"

static uint32_t hele;        // gehele Hz
static uint8_t  dec;         // decimalen (hoogstens 2 cijfers)
static uint8_t  dec_n;       // aantal decimalen tot nu toe
static uint8_t  in_dec;      // we zijn voorbij de punt/komma
static uint8_t  n_cijfers;   // aantal cijfers getypt
static uint8_t  fout;        // er is iets ongeldigs getypt

static void wis(void)
{
    hele = 0;
    dec = 0;
    dec_n = 0;
    in_dec = 0;
    n_cijfers = 0;
    fout = 0;
}

#if FREQ_INPUT_ECHO
#define ECHO(c)  putchar(c)
#else
#define ECHO(c)  ((void)0)
#endif

static uint16_t rx_totaal;       // aantal bytes dat tot nu toe uit serialF0 is gelezen

void freq_input_debug(void)
{
#if FREQ_INPUT_DEBUG
    printf("fi-v3 rx=%u\n", (unsigned)rx_totaal);
#endif
}

uint8_t freq_input_poll(uint32_t *f_centihz)
{
    uint16_t r;
    while ((r = FREQ_RX_LEES()) != FREQ_RX_GEEN) {
        uint8_t c = (uint8_t)r;
        rx_totaal++;
#if FREQ_INPUT_DEBUG
        printf("[RX %02X]", (unsigned)c);
#endif

        if (c >= '0' && c <= '9') {
            n_cijfers++;
            if (in_dec) {
                if (dec_n < 2) {                    // meer dan 2 decimalen: negeren
                    dec = (uint8_t)(dec * 10 + (c - '0'));
                    dec_n++;
                }
            } else {
                hele = hele * 10 + (c - '0');
                if (hele > 99999UL) fout = 1;       // veel te groot, en voorkomt overflow
            }
            ECHO(c);

        } else if (c == '.' || c == ',') {
            if (in_dec) fout = 1;                   // twee punten
            in_dec = 1;
            ECHO(c);

        } else if (c == '\r' || c == '\n') {
            if (n_cijfers == 0 && !fout) continue;  // lege regel, of de \n na een \r

            uint32_t centi = hele * 100UL + (dec_n == 1 ? dec * 10UL : dec);
            uint8_t  ok = !fout && centi >= FREQ_INPUT_MIN_CENTIHZ
                                && centi <= FREQ_INPUT_MAX_CENTIHZ;
            wis();
            if (ok) {
                *f_centihz = centi;
                return 1;
            }
            printf("\nOngeldig, typ %lu.%02lu ... %lu Hz, bijvoorbeeld 440 of 261.63\n",
                   (unsigned long)(FREQ_INPUT_MIN_CENTIHZ / 100),
                   (unsigned long)(FREQ_INPUT_MIN_CENTIHZ % 100),
                   (unsigned long)(FREQ_INPUT_MAX_CENTIHZ / 100));

        } else if (c == 8 || c == 127) {            // backspace: alles wissen
            wis();
            ECHO('\n');
        }
        // alle andere tekens negeren
    }
    return 0;
}