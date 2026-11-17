#include <avr/io.h>
#include "toetsen.h"

// Octaafknop op PB4
#define OCTAAF_PORT     PORTB
#define OCTAAF_PIN_bm   PIN4_bm
#define OCTAAF_PINCTRL  PIN4CTRL
#define OCTAAF_BIT      12      // plek in de samengestelde toestand

static uint8_t  octaaf         = 0;
static uint16_t vorige_sample  = 0;   // meting van de vorige poll
static uint16_t vorige_stabiel = 0;   // laatst geaccepteerde (stabiele) status

void toetsen_init(void)
{
    PORTA.DIRCLR = 0xFF;                 // PA0..PA7 als ingang
    PORTB.DIRCLR = 0x0F;                 // PB0..PB3 als ingang
    OCTAAF_PORT.DIRCLR = OCTAAF_PIN_bm;  // PB4 als ingang

    // Interne pull-downs
    for (uint8_t i = 0; i < 8; i++)
        (&PORTA.PIN0CTRL)[i] = PORT_OPC_PULLDOWN_gc;
    for (uint8_t i = 0; i < 4; i++)
        (&PORTB.PIN0CTRL)[i] = PORT_OPC_PULLDOWN_gc;
    OCTAAF_PORT.OCTAAF_PINCTRL = PORT_OPC_PULLDOWN_gc;
}

// Alle knoppen in één 16-bit waarde:
// bit 0..7 = PA0..PA7, bit 8..11 = PB0..PB3, bit 12 = octaafknop
static uint16_t lees_knoppen(void)
{
    uint16_t a = PORTA.IN;
    uint16_t b = PORTB.IN & 0x0F;
    uint16_t o = (OCTAAF_PORT.IN & OCTAAF_PIN_bm) ? 1U : 0U;

    return a | (b << 8) | (o << OCTAAF_BIT);
}

toetsen_event_t toetsen_poll(void)
{
    toetsen_event_t ev = {0, 0};
    uint16_t nu = lees_knoppen();

    // Debounce: alleen accepteren als twee polls na elkaar gelijk zijn
    if (nu == vorige_sample) {
        uint16_t nieuw = nu & ~vorige_stabiel;   // laag-naar-hoog overgangen

        ev.toetsen = nieuw & ((1U << AANTAL_TOETSEN) - 1);

        if (nieuw & (1U << OCTAAF_BIT)) {
            ev.octaaf = 1;
            octaaf = (octaaf >= OCTAAF_MAX) ? 0 : octaaf + 1;
        }

        vorige_stabiel = nu;
    }
    vorige_sample = nu;

    return ev;
}

uint8_t toetsen_octaaf(void)
{
    return octaaf;
}

char toetsen_poort(uint8_t toets)
{
    return (toets < 8) ? 'A' : 'B';
}

uint8_t toetsen_pin(uint8_t toets)
{
    return (toets < 8) ? toets : toets - 8;
}