/*!
 * \file    Dac80501.c
 * \brief   Initialisatie van SPID en de DAC80501('s) (zie dac80501.h, DAC_AANTAL)
 */

#ifndef F_CPU
#define F_CPU 32000000UL                // alleen voor _delay_ms; main.c zet dit ook
#endif

#include <util/delay.h>
#include "dac80501.h"

void dac80501_init(void)
{
    // SYNC eerst hoog (DAC niet geselecteerd), daarna pas als uitgang: geen glitch
    DAC1_SYNC_PORT.OUTSET = DAC1_SYNC_bm;
    DAC1_SYNC_PORT.DIRSET = DAC1_SYNC_bm;
#if DAC_AANTAL > 1
    DAC2_SYNC_PORT.OUTSET = DAC2_SYNC_bm;
    DAC2_SYNC_PORT.DIRSET = DAC2_SYNC_bm;
#endif

    // Bij de XMEGA moet je MOSI en SCK zelf als uitgang zetten (SS ook, zie SYNC hierboven).
    // SCK is in rust laag (mode 1). MISO (PD6) blijft ingang.
    DAC_SPI_PORT.OUTCLR = DAC_SCK_bm | DAC_MOSI_bm;
    DAC_SPI_PORT.DIRSET = DAC_SCK_bm | DAC_MOSI_bm;

    // Master, mode 1, f_periph/2 = 16 MHz (DAC kan tot 50 MHz): een frame van 24 bit duurt 1,5 us
    DAC_SPI.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_1_gc |
                   SPI_PRESCALER_DIV4_gc | SPI_CLK2X_bm;

    _delay_ms(1);                       // de DAC heeft na het opstarten 250 us POR-tijd nodig

    for (uint8_t dac = 0; dac < DAC_AANTAL; dac++) {
        dac80501_write_reg(dac, DAC_REG_GAIN, DAC_GAIN_WAARDE);
        _delay_us(1);                   // SYNC minstens 160 ns hoog tussen twee frames
        dac80501_write(dac, 0x8000);    // midscale = stilte
        _delay_us(1);
    }
}
