#include <avr/io.h>
#include "toetsen.h"

// Octaafknop op PB4
#define OCTAAF_PORT     PORTB
#define OCTAAF_PIN_bm   PIN4_bm
#define OCTAAF_PINCTRL  PIN4CTRL
#define OCTAAF_BIT      12     

#define TOETS10_BIT     10
#define TOETS10_PORT    PORTC
#define TOETS10_PIN_bm  PIN0_bm
#define TOETS10_PINCTRL PIN0CTRL

static uint8_t  octaaf         = 0;
static uint16_t vorige_sample  = 0; 
static uint16_t vorige_stabiel = 0;   

void toetsen_init(void)
{
    PORTA.DIRCLR = 0xFF;                 // PA0..PA7 als ingang
    PORTB.DIRCLR = 0x0B;                 // PB0, PB1, PB3 als ingang (PB2 = DAC)
    TOETS10_PORT.DIRCLR = TOETS10_PIN_bm;// PC0 als ingang
    OCTAAF_PORT.DIRCLR = OCTAAF_PIN_bm;  // PB4 als ingang

    // Interne pull-downs
    for (uint8_t i = 0; i < 8; i++)
        (&PORTA.PIN0CTRL)[i] = PORT_OPC_PULLDOWN_gc;
    for (uint8_t i = 0; i < 4; i++)
        if (i != 2)                      // PB2 overslaan, die is de DAC-uitgang
            (&PORTB.PIN0CTRL)[i] = PORT_OPC_PULLDOWN_gc;
    TOETS10_PORT.TOETS10_PINCTRL = PORT_OPC_PULLDOWN_gc;
    OCTAAF_PORT.OCTAAF_PINCTRL = PORT_OPC_PULLDOWN_gc;
}

// Alle knoppen in één 16-bit waarde:
// bit 0..7 = PA0..PA7, bit 8, 9, 11 = PB0, PB1, PB3,
// bit 10 = PC0 (toets 10), bit 12 = octaafknop
static uint16_t lees_knoppen(void)
{
    uint16_t a   = PORTA.IN;
    uint16_t b   = PORTB.IN & 0x0B;      // PB0, PB1, PB3 -> bit 8, 9, 11
    uint16_t t10 = (TOETS10_PORT.IN & TOETS10_PIN_bm) ? 1U : 0U;
    uint16_t o   = (OCTAAF_PORT.IN & OCTAAF_PIN_bm) ? 1U : 0U;

    return a | (b << 8) | (t10 << TOETS10_BIT) | (o << OCTAAF_BIT);
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
    if (toets < 8)             return 'A';
    if (toets == TOETS10_BIT)  return 'C';
    return 'B';
}

uint8_t toetsen_pin(uint8_t toets)
{
    if (toets < 8)             return toets;
    if (toets == TOETS10_BIT)  return 0;
    return toets - 8;
}
