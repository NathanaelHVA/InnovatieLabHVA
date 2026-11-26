/*!
 * \file    liedje.c
 * \brief   Liedjes afspelen op de synth: Vader Jakob en He's a Pirate (zie liedje.h)
 */

#include "liedje.h"
#include "frequentie.h"

/* ---------- instellingen ---------- */
#define GAT_MS          40      // gate zo lang laag aan het eind van elke noot, zodat de envelope opnieuw start
#define HERHALEN        1       // 1 = na het einde opnieuw beginnen, 0 = één keer spelen
// Tempo en octaaf staan per liedje in de lijst 'liedjes' onderaan de melodieën.

/* ---------- noten ---------- */
// toets zoals op je keyboard (0 = C, 2 = D, 4 = E, 5 = F, 7 = G, 9 = A, 10 = Bes, 11 = B),
// oct = t.o.v. het octaaf van het liedje, duur in achtsten:
//   1 = achtste, 2 = kwart, 3 = kwart met punt, 4 = halve, 6 = halve met punt, 8 = hele
#define RUST  0xFF

typedef struct {
    uint8_t toets;
    int8_t  oct;
    uint8_t achtsten;
} noot_t;

/* ---------- Vader Jakob (4/4, C groot) ---------- */
static const noot_t vader_jakob[] = {
    // Vader Jakob, vader Jakob
    {0, 0, 2}, {2, 0, 2}, {4, 0, 2}, {0, 0, 2},
    {0, 0, 2}, {2, 0, 2}, {4, 0, 2}, {0, 0, 2},
    // slaapt gij nog, slaapt gij nog
    {4, 0, 2}, {5, 0, 2}, {7, 0, 4},
    {4, 0, 2}, {5, 0, 2}, {7, 0, 4},
    // alle klokken luiden, alle klokken luiden
    {7, 0, 1}, {9, 0, 1}, {7, 0, 1}, {5, 0, 1}, {4, 0, 2}, {0, 0, 2},
    {7, 0, 1}, {9, 0, 1}, {7, 0, 1}, {5, 0, 1}, {4, 0, 2}, {0, 0, 2},
    // bim bam bom, bim bam bom (de lage G zit een octaaf lager)
    {0, 0, 2}, {7, -1, 2}, {0, 0, 4},
    {0, 0, 2}, {7, -1, 2}, {0, 0, 4},
    // één maat stilte voor het opnieuw begint
    {RUST, 0, 8},
};

/* ---------- He's a Pirate (6/8, d klein) ---------- */
// Pirates of the Caribbean (Klaus Badelt / Hans Zimmer): thema A twee keer, dan thema B twee keer.
// Noten naar de buzzer-versie van Xitang Zhao (github.com/xitanggg/-Pirates-of-the-Caribbean-Theme-Song,
// MIT-licentie), van a klein terug naar het origineel in d klein gezet en elke maat precies 6 achtsten gemaakt.
//
// Elke regel is één maat. In het commentaar is elk teken één achtste:
//   letter = noot begint, '-' = noot klinkt door, '.' = rust
// De melodie loopt van de lage A (220 Hz, oct -1) tot de hoge C (523 Hz, oct 1).
static const noot_t piraat[] = {
    // ===== thema A, 1e keer =====
    {9, -1, 1}, {0, 0, 1},                                              // opmaat:  A C
    {2, 0, 2}, {2, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {4, 0, 1},           // D - D . D E
    {5, 0, 2}, {5, 0, 1}, {RUST, 0, 1}, {5, 0, 1}, {7, 0, 1},           // F - F . F G
    {4, 0, 2}, {4, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {0, 0, 1},           // E - E . D C
    {2, 0, 3}, {RUST, 0, 1}, {9, -1, 1}, {0, 0, 1},                     // D - - . A C

    {2, 0, 2}, {2, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {4, 0, 1},           // D - D . D E
    {5, 0, 2}, {5, 0, 1}, {RUST, 0, 1}, {5, 0, 1}, {7, 0, 1},           // F - F . F G
    {4, 0, 2}, {4, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {0, 0, 1},           // E - E . D C
    {2, 0, 3}, {RUST, 0, 1}, {9, -1, 1}, {0, 0, 1},                     // D - - . A C

    {2, 0, 2}, {2, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {5, 0, 1},           // D - D . D F
    {7, 0, 2}, {7, 0, 1}, {RUST, 0, 1}, {7, 0, 1}, {9, 0, 1},           // G - G . G A
    {10, 0, 2}, {10, 0, 1}, {RUST, 0, 1}, {9, 0, 1}, {7, 0, 1},         // Bes - Bes . A G
    {9, 0, 1}, {2, 0, 2}, {RUST, 0, 1}, {2, 0, 1}, {4, 0, 1},           // A D - . D E

    {5, 0, 2}, {5, 0, 1}, {RUST, 0, 1}, {7, 0, 2},                      // F - F . G -
    {9, 0, 1}, {2, 0, 2}, {RUST, 0, 1}, {2, 0, 1}, {5, 0, 1},           // A D - . D F
    {4, 0, 2}, {4, 0, 1}, {RUST, 0, 1}, {5, 0, 1}, {2, 0, 1},           // E - E . F D
    {4, 0, 3}, {RUST, 0, 3},                                            // E - - . . .

    // ===== thema A, 2e keer (begint zonder opmaat) =====
    {2, 0, 2}, {2, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {4, 0, 1},           // D - D . D E
    {5, 0, 2}, {5, 0, 1}, {RUST, 0, 1}, {5, 0, 1}, {7, 0, 1},           // F - F . F G
    {4, 0, 2}, {4, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {0, 0, 1},           // E - E . D C
    {2, 0, 3}, {RUST, 0, 1}, {9, -1, 1}, {0, 0, 1},                     // D - - . A C

    {2, 0, 2}, {2, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {4, 0, 1},           // D - D . D E
    {5, 0, 2}, {5, 0, 1}, {RUST, 0, 1}, {5, 0, 1}, {7, 0, 1},           // F - F . F G
    {4, 0, 2}, {4, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {0, 0, 1},           // E - E . D C
    {2, 0, 3}, {RUST, 0, 1}, {9, -1, 1}, {0, 0, 1},                     // D - - . A C

    {2, 0, 2}, {2, 0, 1}, {RUST, 0, 1}, {2, 0, 1}, {5, 0, 1},           // D - D . D F
    {7, 0, 2}, {7, 0, 1}, {RUST, 0, 1}, {7, 0, 1}, {9, 0, 1},           // G - G . G A
    {10, 0, 2}, {10, 0, 1}, {RUST, 0, 1}, {9, 0, 1}, {7, 0, 1},         // Bes - Bes . A G
    {9, 0, 1}, {2, 0, 2}, {RUST, 0, 1}, {2, 0, 1}, {4, 0, 1},           // A D - . D E

    {5, 0, 2}, {5, 0, 1}, {RUST, 0, 1}, {7, 0, 2},                      // F - F . G -
    {9, 0, 1}, {2, 0, 2}, {RUST, 0, 1}, {2, 0, 1}, {5, 0, 1},           // A D - . D F
    {4, 0, 2}, {4, 0, 1}, {RUST, 0, 1}, {5, 0, 1}, {2, 0, 1},           // E - E . F D
    {4, 0, 3}, {RUST, 0, 3},                                            // E - - . . .

    // ===== thema B, 1e keer =====
    {9, 0, 6},                                                          // A - - - - -
    {10, 0, 6},                                                         // Bes - - - - -
    {9, 0, 1}, {9, 0, 1}, {RUST, 0, 1}, {0, 1, 1}, {RUST, 0, 1}, {9, 0, 1}, // A A . C . A
    {7, 0, 6},                                                          // G - - - - -

    {7, 0, 6},                                                          // G - - - - -
    {5, 0, 6},                                                          // F - - - - -
    {4, 0, 1}, {5, 0, 1}, {RUST, 0, 1}, {4, 0, 1}, {RUST, 0, 1}, {2, 0, 4}, // E F . E . D  (D loopt door)
    {RUST, 0, 3},                                                       // - - - . . .

    // ===== thema B, 2e keer =====
    {9, 0, 6},                                                          // A - - - - -
    {10, 0, 6},                                                         // Bes - - - - -
    {9, 0, 1}, {9, 0, 1}, {RUST, 0, 1}, {0, 1, 1}, {RUST, 0, 1}, {9, 0, 1}, // A A . C . A
    {7, 0, 6},                                                          // G - - - - -

    {7, 0, 6},                                                          // G - - - - -
    {5, 0, 6},                                                          // F - - - - -
    {4, 0, 1}, {5, 0, 1}, {RUST, 0, 1}, {4, 0, 1}, {RUST, 0, 1}, {2, 0, 4}, // E F . E . D  (D loopt door)

    // rest van de maat + één maat stilte, tot aan de opmaat (A C) waarmee het opnieuw begint
    {RUST, 0, 7},                                                       // - - - . . .   . . . .
};

/* ---------- de lijst met liedjes ---------- */
typedef struct {
    const char   *naam;
    const noot_t *noten;
    uint16_t      aantal;
    uint16_t      achtste_ms;   // tempo: duur van een achtste noot
    uint8_t       octaaf;       // 1 -> C = 261,6 Hz (zoals octaaf 1 op het keyboard)
} liedje_t;

#define AANTAL(x)  ((uint16_t)(sizeof(x) / sizeof((x)[0])))

static const liedje_t liedjes[LIEDJE_AANTAL] = {
    // 250 ms per achtste = 120 kwarten per minuut
    [LIEDJE_VADER_JAKOB] = { "Vader Jakob",   vader_jakob, AANTAL(vader_jakob), 250, 1 },
    // 190 ms per achtste = 105 tellen (kwart met punt) per minuut, ongeveer zoals de film; kleiner = sneller
    [LIEDJE_PIRAAT]      = { "He's a Pirate", piraat,      AANTAL(piraat),      190, 1 },
};

/* ---------- afspelen ---------- */
static const liedje_t *lied = &liedjes[LIEDJE_VADER_JAKOB];   // gekozen liedje
static uint16_t idx;            // welke noot
static uint32_t ms;             // hoe lang de noot al klinkt
static int      bezig;
static int      gate_nu;

static uint32_t duur_ms(const noot_t *n)
{
    return (uint32_t)n->achtsten * lied->achtste_ms;
}

void liedje_start(liedje_nr_t nr)
{
    if (nr < 0 || nr >= LIEDJE_AANTAL) return;
    lied = &liedjes[nr];
    idx = 0;
    ms = 0;
    bezig = 1;
}

void liedje_stop(void)
{
    bezig = 0;
    gate_nu = 0;
}

int liedje_bezig(void)
{
    return bezig;
}

liedje_nr_t liedje_huidig(void)
{
    return (liedje_nr_t)(lied - liedjes);
}

const char *liedje_naam(void)
{
    return lied->naam;
}

int liedje_gate(void)
{
    return gate_nu;
}

int liedje_tik(uint32_t tik_ms, uint32_t *f_centi)
{
    if (!bezig) return 0;

    const noot_t *n = &lied->noten[idx];
    int nieuw = 0;

    // Begin van een noot: frequentie doorgeven (een rust verandert niks aan de toonhoogte)
    if (ms == 0 && n->toets != RUST) {
        *f_centi = frequentie_centihz(n->toets, (uint8_t)(lied->octaaf + n->oct));
        nieuw = 1;
    }

    // Gate hoog tijdens de noot, behalve de laatste GAT_MS
    gate_nu = (n->toets != RUST) && (ms + GAT_MS < duur_ms(n));

    // Tijd laten lopen, en zo nodig naar de volgende noot
    ms += tik_ms;
    if (ms >= duur_ms(n)) {
        ms = 0;
        if (++idx >= lied->aantal) {
            idx = 0;
            if (!HERHALEN) liedje_stop();
        }
    }
    return nieuw;
}
