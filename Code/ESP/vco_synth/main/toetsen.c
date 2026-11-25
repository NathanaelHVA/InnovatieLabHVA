#include "toetsen.h"
#include "pinnen.h"
#include "driver/gpio.h"

// Plek van de overige knoppen in de samengestelde toestand (bit 0..11 = toetsen)
#define OCTAAF_BIT      12
#define GOLF1_BIT       13
#define GOLF2_BIT       14
#define RESERVE_BIT     15

static const int toets_gpio[AANTAL_TOETSEN] = {
    TOETS0_GPIO, TOETS1_GPIO, TOETS2_GPIO, TOETS3_GPIO, TOETS4_GPIO,  TOETS5_GPIO,
    TOETS6_GPIO, TOETS7_GPIO, TOETS8_GPIO, TOETS9_GPIO, TOETS10_GPIO, TOETS11_GPIO,
};

static uint8_t  octaaf         = 0;
static uint16_t vorige_sample  = 0;   // meting van de vorige poll
static uint16_t vorige_stabiel = 0;   // laatst geaccepteerde (stabiele) status

void toetsen_init(void)
{
    uint64_t masker = (1ULL << OCTAAF_GPIO) | (1ULL << GOLF_VCO1_GPIO) |
                      (1ULL << GOLF_VCO2_GPIO) | (1ULL << RESERVE_GPIO);
    for (uint8_t i = 0; i < AANTAL_TOETSEN; i++)
        masker |= 1ULL << toets_gpio[i];

    // Allemaal ingang met interne pull-down (zoals op de XMEGA): ingedrukt = 3V3 = 1
    gpio_config_t io = {
        .pin_bit_mask = masker,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
}

// Alle knoppen in één 16-bit waarde:
// bit 0..11 = toets 0..11, bit 12 = octaaf, bit 13/14 = golf VCO 1/2, bit 15 = reserve
static uint16_t lees_knoppen(void)
{
    uint16_t s = 0;
    for (uint8_t i = 0; i < AANTAL_TOETSEN; i++)
        if (gpio_get_level(toets_gpio[i]))
            s |= 1U << i;
    if (gpio_get_level(OCTAAF_GPIO))    s |= 1U << OCTAAF_BIT;
    if (gpio_get_level(GOLF_VCO1_GPIO)) s |= 1U << GOLF1_BIT;
    if (gpio_get_level(GOLF_VCO2_GPIO)) s |= 1U << GOLF2_BIT;
    if (gpio_get_level(RESERVE_GPIO))   s |= 1U << RESERVE_BIT;
    return s;
}

toetsen_event_t toetsen_poll(void)
{
    toetsen_event_t ev = {0, 0, 0, 0};
    uint16_t nu = lees_knoppen();

    // Debounce: alleen accepteren als twee polls na elkaar gelijk zijn
    if (nu == vorige_sample) {
        uint16_t nieuw = nu & ~vorige_stabiel;   // laag-naar-hoog overgangen

        ev.toetsen = nieuw & ((1U << AANTAL_TOETSEN) - 1);

        if (nieuw & (1U << OCTAAF_BIT)) {
            ev.octaaf = 1;
            octaaf = (octaaf >= OCTAAF_MAX) ? 0 : octaaf + 1;
        }
        if (nieuw & (1U << GOLF1_BIT))   ev.golf |= 1;
        if (nieuw & (1U << GOLF2_BIT))   ev.golf |= 2;
        if (nieuw & (1U << RESERVE_BIT)) ev.reserve = 1;

        vorige_stabiel = nu;
    }
    vorige_sample = nu;

    return ev;
}

uint8_t toetsen_octaaf(void)
{
    return octaaf;
}

uint16_t toetsen_vast(void)
{
    return vorige_stabiel & ((1U << AANTAL_TOETSEN) - 1);
}

int toetsen_gpio(uint8_t toets)
{
    return (toets < AANTAL_TOETSEN) ? toets_gpio[toets] : -1;
}
