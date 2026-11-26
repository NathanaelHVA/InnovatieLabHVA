/*!
 * \file    freq_input.c
 * \brief   Frequentie of volume intypen via de seriële terminal (zie freq_input.h)
 *
 *   440     + Enter  -> frequentie 440 Hz         (geeft FREQ_INPUT_FREQ terug)
 *   261.63  + Enter  -> frequentie 261,63 Hz
 *   v50     + Enter  -> volume 50 %, v0 .. v100   (geeft FREQ_INPUT_VOLUME terug)
 *   j       + Enter  -> Vader Jakob aan/uit       (geeft FREQ_INPUT_LIEDJE terug, waarde 'j')
 *   p       + Enter  -> He's a Pirate aan/uit     (geeft FREQ_INPUT_LIEDJE terug, waarde 'p')
 */

#include <stdio.h>
#include "freq_input.h"

static uint32_t hele;        // gehele Hz (of het volume in %)
static uint8_t  dec;         // decimalen (hoogstens 2 cijfers)
static uint8_t  dec_n;       // aantal decimalen tot nu toe
static uint8_t  in_dec;      // we zijn voorbij de punt/komma
static uint8_t  n_cijfers;   // aantal cijfers getypt
static uint8_t  fout;        // er is iets ongeldigs getypt
static uint8_t  is_vol;      // regel begon met 'v': het is een volume
static uint8_t  lied;        // regel is een liedje: 'j' of 'p' (0 = geen liedje)

static void wis(void)
{
    hele = 0;
    dec = 0;
    dec_n = 0;
    in_dec = 0;
    n_cijfers = 0;
    fout = 0;
    is_vol = 0;
    lied = 0;
}

#if FREQ_INPUT_ECHO
#define ECHO(c)  do { putchar(c); fflush(stdout); } while (0)
#else
#define ECHO(c)  ((void)0)
#endif

static uint16_t rx_totaal;       // aantal bytes dat tot nu toe uit serialF0 is gelezen

void freq_input_debug(void)
{
#if FREQ_INPUT_DEBUG
    printf("fi-v4 rx=%u\n", (unsigned)rx_totaal);
#endif
}

uint8_t freq_input_poll(uint32_t *waarde)
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

        } else if ((c == 'j' || c == 'J' || c == 'p' || c == 'P') && n_cijfers == 0 && !in_dec && !is_vol && !lied) {
            lied = (c == 'p' || c == 'P') ? 'p' : 'j';  // 'j': Vader Jakob, 'p': He's a Pirate
            ECHO(c);

        } else if ((c == 'v' || c == 'V') && n_cijfers == 0 && !in_dec && !is_vol) {
            is_vol = 1;                             // 'v' vooraan: dit wordt een volume
            ECHO(c);

        } else if (c == '.' || c == ',') {
            if (in_dec || is_vol) fout = 1;         // twee punten, of een komma in een volume
            in_dec = 1;
            ECHO(c);

        } else if (c == '\r' || c == '\n') {
            if (n_cijfers == 0 && !fout && !is_vol && !lied) continue;  // lege regel, of de \n na een \r

            if (lied) {
                uint8_t ok = !fout && n_cijfers == 0;
                uint8_t l  = lied;
                wis();
                if (ok) {
                    *waarde = l;                    // 'j' of 'p'
                    return FREQ_INPUT_LIEDJE;
                }
                printf("\nOngeldig, typ alleen j of p + Enter\n");
                continue;
            }

            if (is_vol) {
                uint8_t ok = !fout && n_cijfers > 0 && hele <= 100;
                uint32_t v = hele;
                wis();
                if (ok) {
                    *waarde = v;
                    return FREQ_INPUT_VOLUME;
                }
                printf("\nOngeldig volume, typ v0 ... v100, bijvoorbeeld v50\n");
                continue;
            }

            uint32_t centi = hele * 100UL + (dec_n == 1 ? dec * 10UL : dec);
            uint8_t  ok = !fout && centi >= FREQ_INPUT_MIN_CENTIHZ
                                && centi <= FREQ_INPUT_MAX_CENTIHZ;
            wis();
            if (ok) {
                *waarde = centi;
                return FREQ_INPUT_FREQ;
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
    return FREQ_INPUT_NIETS;
}
