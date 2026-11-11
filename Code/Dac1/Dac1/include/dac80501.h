/*!
 * \file    dac80501.h
 * \brief   Driver voor één of twee DAC80501 (16 bit, SPI) op SPID (aantal: DAC_AANTAL)
 *
 * Pinnen:
 *   PD7 = SCLK  (alle DAC's)
 *   PD5 = SDIN  (alle DAC's)
 *   PD4 = SYNC  van DAC 1  (dit is de SS-pin van SPID, hier als gewone GPIO gebruikt)
 *   PF4 = SYNC  van DAC 2 (alleen bij DAC_AANTAL 2)
 *   PD6 = MISO, niet gebruikt (de DAC80501 heeft geen SDO)
 * Hardware: SPI2C-pin van elke DAC naar GND (= SPI-mode), pull-up op SYNC.
 *
 * Frame van 24 bit: SYNC laag, 8 bit registeradres + 16 bit data, SYNC hoog.
 * De DAC werkt bij de stijgende flank van SYNC bij en neemt data over op de
 * dalende flank van SCLK, dus SPI mode 1 (CPOL 0, CPHA 1).
 */
#ifndef DAC80501_H
#define DAC80501_H

#include <avr/io.h>
#include <stdint.h>

/* ---------- pinnen: hier aanpassen ---------- */
#define DAC_SPI           SPID
#define DAC_SPI_PORT      PORTD
#define DAC_SCK_bm        PIN7_bm     // PD7
#define DAC_MOSI_bm       PIN5_bm     // PD5

#define DAC1_SYNC_PORT    PORTD
#define DAC1_SYNC_bm      PIN4_bm     // PD4
#define DAC2_SYNC_PORT    PORTF
#define DAC2_SYNC_bm      PIN4_bm     // PF4

/* ---------- uitgangsbereik ----------
 * Volle schaal = 2,5 V (interne referentie) / (REF_DIV ? 2 : 1) * (BUFF_GAIN ? 2 : 1)
 *   REF_DIV 1, BUFF_GAIN 1 -> 0 ... 2,5 V   (standaard hier)
 *   REF_DIV 1, BUFF_GAIN 0 -> 0 ... 1,25 V
 *   REF_DIV 0, BUFF_GAIN 1 -> 0 ... 5 V     (vraagt VDD >= 5 V; zakt de voeding iets, dan
 *                                            gaat de uitgang door een referentie-alarm naar 0 V)
 */
#define DAC_REF_DIV       1
#define DAC_BUFF_GAIN     1

/* ---------- registers ---------- */
#define DAC_REG_GAIN      0x04
#define DAC_REG_DATA      0x08
#define DAC_GAIN_WAARDE   ((uint16_t)((((DAC_REF_DIV) & 1) << 8) | ((DAC_BUFF_GAIN) & 1)))

/* Aantal aangesloten DAC's: 1 of 2 (ook te zetten met -DDAC_AANTAL=2).
 *   1 -> alleen DAC 1 (SYNC = PD4). PF4 wordt niet aangeraakt.
 *   2 -> ook DAC 2 (SYNC = PF4). */
#ifndef DAC_AANTAL
#define DAC_AANTAL        1
#endif
#define DAC_EXT1          0
#define DAC_EXT2          1

/*! \brief SPI aanzetten, beide DAC's configureren en op midscale zetten (stilte) */
void dac80501_init(void);

/* Onderstaande functies zijn altijd inline: een gewone functieaanroep in een ISR kost
 * ~100 extra cycles (de compiler bewaart dan bijna alle registers). */
#define DAC_INLINE static inline __attribute__((always_inline))

DAC_INLINE void dac_spi_byte(uint8_t b)
{
    DAC_SPI.DATA = b;
    while (!(DAC_SPI.STATUS & SPI_IF_bm)) ;
    (void)DAC_SPI.DATA;                 // STATUS + DATA lezen wist de IF-vlag
}

DAC_INLINE void dac_sync_laag(uint8_t dac)
{
#if DAC_AANTAL > 1
    if (dac != DAC_EXT1) { DAC2_SYNC_PORT.OUTCLR = DAC2_SYNC_bm; return; }
#else
    (void)dac;
#endif
    DAC1_SYNC_PORT.OUTCLR = DAC1_SYNC_bm;
}

DAC_INLINE void dac_sync_hoog(uint8_t dac)
{
#if DAC_AANTAL > 1
    if (dac != DAC_EXT1) { DAC2_SYNC_PORT.OUTSET = DAC2_SYNC_bm; return; }
#else
    (void)dac;
#endif
    DAC1_SYNC_PORT.OUTSET = DAC1_SYNC_bm;
}

/*! \brief Eén register schrijven (24-bit frame) */
DAC_INLINE void dac80501_write_reg(uint8_t dac, uint8_t reg, uint16_t data)
{
    dac_sync_laag(dac);
    dac_spi_byte(reg);
    dac_spi_byte((uint8_t)(data >> 8));
    dac_spi_byte((uint8_t)data);
    dac_sync_hoog(dac);
}

/*! \brief Nieuwe uitgangswaarde (0 ... 65535) naar DAC 1 of DAC 2 */
DAC_INLINE void dac80501_write(uint8_t dac, uint16_t code)
{
    dac80501_write_reg(dac, DAC_REG_DATA, code);
}

#endif // DAC80501_H
