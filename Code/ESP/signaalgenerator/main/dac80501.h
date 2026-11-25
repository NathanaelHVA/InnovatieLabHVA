/*!
 * \file    dac80501.h
 * \brief   Twee DAC80501 (16 bit, SPI) tegelijk aansturen vanaf een ESP32-S3
 *
 * De SPI-bits worden door de CPU zelf gemaakt met "dedicated GPIO" (pinnen die de CPU
 * direct kan schakelen). Beide DAC's delen SCLK en SYNC en hebben elk een eigen SDIN,
 * dus beide DAC's krijgen in één frame van 24 klokpulsen tegelijk een nieuwe waarde.
 *
 * Bedrading:
 *   GPIO48 = SCLK   -> SCLK van DAC 1 en DAC 2
 *   GPIO47 = SDIN1  -> SDIN van DAC 1
 *   GPIO45 = SDIN2  -> SDIN van DAC 2  (strapping-pin: GEEN pull-up op deze lijn!)
 *   GPIO21 = SYNC   -> SYNC van DAC 1 en DAC 2 (pull-up, zoals eerst)
 *   Zit de RGB-LED van je board op GPIO47 of 48, dan knippert die mee: onschuldig.
 *   SPI2C-pin van elke DAC naar GND (SPI-mode), VDD van de DAC's op 3V3.
 *
 * Frame van 24 bit: SYNC laag, 8 bit registeradres + 16 bit data, SYNC hoog.
 * Data verandert bij de stijgende flank van SCLK, de DAC leest bij de dalende (SPI mode 1).
 *
 * Let op: dac80501_init() en de write-functies moeten op DEZELFDE core draaien
 * (een dedicated-GPIO-bundel hoort bij de core die hem aanmaakt).
 */
#ifndef DAC80501_H
#define DAC80501_H

#include <stdint.h>

/* ---------- pinnen: hier aanpassen ---------- */
#define DAC_PIN_SCLK      48
#define DAC_PIN_SDIN1     47
#define DAC_PIN_SDIN2     0      // strapping-pin: moet laag zijn bij opstarten, dus geen pull-up
#define DAC_PIN_SYNC      21

/* ---------- uitgangsbereik (zelfde als op de XMEGA) ----------
 * Volle schaal = 2,5 V (interne referentie) / (REF_DIV ? 2 : 1) * (BUFF_GAIN ? 2 : 1)
 *   REF_DIV 1, BUFF_GAIN 1 -> 0 ... 2,5 V   (standaard hier)
 *   REF_DIV 1, BUFF_GAIN 0 -> 0 ... 1,25 V
 */
#define DAC_REF_DIV       1
#define DAC_BUFF_GAIN     1

/* ---------- registers ---------- */
#define DAC_REG_GAIN      0x04
#define DAC_REG_DATA      0x08
#define DAC_GAIN_WAARDE   ((uint16_t)((((DAC_REF_DIV) & 1) << 8) | ((DAC_BUFF_GAIN) & 1)))

/*! \brief Pinnen klaarzetten, beide DAC's configureren en op midscale zetten (stilte) */
void dac80501_init(void);

/*! \brief Eén register in beide DAC's schrijven, elk met een eigen waarde */
void dac80501_write_reg2(uint8_t reg, uint16_t data1, uint16_t data2);

/*! \brief Nieuwe uitgangswaarden (0 ... 65535) naar DAC 1 en DAC 2 tegelijk */
static inline void dac80501_write2(uint16_t code1, uint16_t code2)
{
    dac80501_write_reg2(DAC_REG_DATA, code1, code2);
}

#endif // DAC80501_H
