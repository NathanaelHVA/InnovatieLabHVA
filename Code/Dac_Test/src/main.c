/*!
 * \file    main.c
 * \brief   Pure signaalgenerator: 1 of 2 externe DAC80501 (SPI), 4 golfvormen, frequentie intypen
 *
 * Zo werkt het, van typen tot signaal:
 *   1. Jij typt in de terminal bijvoorbeeld 440 + Enter.
 *   2. freq_input_poll() maakt van die tekens een getal in centi-Hz (440 Hz = 44000).
 *   3. setFreq_centihz() rekent dat om naar een "stap" per sample (vco_step).
 *   4. Een timer (TCC0) tikt FS keer per seconde. In die ISR doen we elke tik:
 *          fase += stap  ->  waarde opzoeken in de golftabel  ->  naar de DAC sturen
 *      Een grote stap = de golf wordt sneller doorlopen = hogere frequentie.
 *
 * Wat je kunt instellen:
 *   - aantal DAC's (1 of 2)  : DAC_AANTAL in dac80501.h (of -DDAC_AANTAL=2)
 *   - golfvorm per VCO       : VCO1_GOLF en VCO2_GOLF hieronder
 *   - toonhoogte van VCO 2   : VCO2_RATIO_Q8 hieronder
 *   - frequentie             : intypen in de terminal, of START_FREQ_CENTIHZ voor bij het opstarten
 *
 * Pinnen: zie dac80501.h (SCLK = PD7, SDIN = PD5, SYNC DAC 1 = PD4, SYNC DAC 2 = PF4)
 */

#define F_CPU 32000000UL               // vóór de includes, één keer (klok gaat naar 32 MHz)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "serialF0.h"
#include "dac80501.h"
#include "freq_input.h"

/* ---------- instellingen ---------- */

// Samplefrequentie van de VCO's. Per sample gaan er twee SPI-frames van 24 bit over de bus
// (3 us) en de ISR zelf kost ongeveer 9 us (geschat uit de assembly, niet gemeten).
// Bij 100 kHz is de CPU dan bijna vol (10 us per sample); 90 kHz is nog net werkbaar.
// (Dat geldt voor 2 DAC's. Bij DAC_AANTAL 1 gaat er één frame over de bus en wordt maar één VCO berekend.)
#define FS 90000UL

// 1 = lineair interpoleren tussen twee tabelpunten (veel schoner signaal, kost per DAC ongeveer 1-2 us
//     extra in de ISR, niet gemeten); 0 = gewoon het dichtstbijzijnde tabelpunt (blokkerig, ~37 dB SNR)
#define VCO_INTERPOLATIE 1

// Golfvorm per VCO: GOLF_SINUS, GOLF_BLOK, GOLF_ZAAGTAND of GOLF_DRIEHOEK
#define VCO1_GOLF       GOLF_SINUS
#define VCO2_GOLF       GOLF_ZAAGTAND

// Toonhoogte van VCO 2 t.o.v. VCO 1 in 1/256: 256 = gelijk, 512 = een octaaf hoger, 257 = lichte detune
#define VCO2_RATIO_Q8   256UL

// Frequentie bij het opstarten, in centi-Hz (100000 = 1000,00 Hz)
#define START_FREQ_CENTIHZ  100000UL

/* ---------- afgeleide waarden (niet aanpassen) ---------- */

// De timer telt in hele klokpulsen, dus de echte samplefrequentie wijkt een beetje af van FS
// (32 MHz / 355 = 90140 Hz i.p.v. 90000 Hz). Voor de stap rekenen we met de echte waarde,
// anders klopt de frequentie niet precies.
#define FS_DIV    (F_CPU / FS)         // klokpulsen per sample
#define FS_ECHT   (F_CPU / FS_DIV)     // echte samplefrequentie in Hz

/* ---------- golfvormen en VCO's ---------- */

typedef enum { GOLF_SINUS, GOLF_BLOK, GOLF_ZAAGTAND, GOLF_DRIEHOEK, GOLF_AANTAL } golf_t;

static const char *golf_naam[GOLF_AANTAL] = { "sinus", "blok", "zaagtand", "driehoek" };

static uint16_t golf_tabel[GOLF_AANTAL][256];   // 16-bit tabellen voor de DAC80501

static uint32_t                   vco_phase[2];  // alleen de ISR schrijft hierin
static volatile uint32_t          vco_step[2];
static const uint16_t * volatile  vco_lut[2];

// Tabelwaarde bij een 32-bit fase: bovenste 8 bit = tabelindex, de 8 bit daaronder = fractie
static inline uint16_t lut_waarde(const uint16_t *lut, uint32_t phase) {
#if VCO_INTERPOLATIE
    uint8_t  i  = (uint8_t)(phase >> 24);
    uint8_t  fr = (uint8_t)(phase >> 16);
    uint16_t a  = lut[i];
    uint16_t b  = lut[(uint8_t)(i + 1)];            // na punt 255 weer naar punt 0
    int32_t  d  = (int32_t)b - (int32_t)a;
    return (uint16_t)(a + ((d * fr) >> 8));
#else
    return lut[phase >> 24];
#endif
}

// De sampleklok: elke tik één nieuwe waarde naar elke DAC
ISR(TCC0_OVF_vect) {
    vco_phase[0] += vco_step[0];
    dac80501_write(DAC_EXT1, lut_waarde(vco_lut[0], vco_phase[0]));
#if DAC_AANTAL > 1
    vco_phase[1] += vco_step[1];
    dac80501_write(DAC_EXT2, lut_waarde(vco_lut[1], vco_phase[1]));
#endif
}

// frequentie in centi-Hz (440 Hz = 44000); VCO 2 volgt VCO 1 met VCO2_RATIO_Q8
// stap = f / FS * 2^32  (de fase loopt van 0 tot 2^32 = één periode)
static void setFreq_centihz(uint32_t f_centi) {
    uint32_t s1 = (uint32_t)(((uint64_t)f_centi << 32) / (FS_ECHT * 100UL));
    uint32_t s2 = (uint32_t)(((uint64_t)s1 * VCO2_RATIO_Q8) >> 8);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { vco_step[0] = s1; vco_step[1] = s2; }
}

// golfvorm van een VCO wisselen (vco = 0 of 1)
static void vco_set_golf(uint8_t vco, golf_t golf) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { vco_lut[vco] = golf_tabel[golf]; }
}

// Een via de seriële poort ingetypte frequentie (bijvoorbeeld "440" + Enter) meteen toepassen
static void serieel_frequentie(void) {
    uint32_t f;
    if (freq_input_poll(&f)) {
        setFreq_centihz(f);
        printf("\nFrequentie: %lu.%02lu Hz\n",
               (unsigned long)(f / 100),
               (unsigned long)(f % 100));
    }
}

static void klok_32mhz(void) {
    OSC.CTRL |= OSC_RC32MEN_bm;
    while (!(OSC.STATUS & OSC_RC32MRDY_bm));
    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
}

static void golf_init(void) {
    for (int i = 0; i < 256; i++) {
        golf_tabel[GOLF_SINUS][i]    = (uint16_t)(32767.5 + 32767.0 * sin(2 * M_PI * i / 256.0));
        golf_tabel[GOLF_BLOK][i]     = (i < 128) ? 65535 : 0;
        golf_tabel[GOLF_ZAAGTAND][i] = (uint16_t)((uint32_t)i * 65535UL / 255UL);
        golf_tabel[GOLF_DRIEHOEK][i] = (uint16_t)((uint32_t)((i < 128) ? i : 255 - i) * 65535UL / 127UL);
    }
    vco_set_golf(0, VCO1_GOLF);
    vco_set_golf(1, VCO2_GOLF);

    // Timer TCC0 als sampleklok: ISR elke FS_DIV klokpulsen
    TCC0.PER = FS_DIV - 1;
    TCC0.INTCTRLA = TC_OVFINTLVL_HI_gc;
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
    PMIC.CTRL |= PMIC_HILVLEN_bm;
}

/* ---------- main ---------- */

int main(void)
{
    klok_32mhz();
    init_stream(F_CPU);
    dac80501_init();           // SPID + de DAC('s) instellen (DAC_AANTAL)
    golf_init();               // tabellen + sampleklok (de ISR schrijft naar de DAC's)
    setFreq_centihz(START_FREQ_CENTIHZ);
    sei();                     // pas hierna kan printf versturen (TX werkt met interrupts)

    printf("\nSignaalgenerator, %u DAC(s)\n", (unsigned)DAC_AANTAL);
    printf("DAC 1: %s", golf_naam[VCO1_GOLF]);
#if DAC_AANTAL > 1
    printf("   DAC 2: %s", golf_naam[VCO2_GOLF]);
#endif
    printf("\nTyp een frequentie (440 of 261.63) + Enter\n");

    while (1) {
        serieel_frequentie();  // niet-blokkerend: kijkt of er nieuwe tekens zijn
    }
}
