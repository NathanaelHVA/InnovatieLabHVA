/*!
 * \file    freq_input.h
 * \brief   Frequentie of volume intypen via de seriële terminal (ESP32-S3, ESP-IDF)
 *
 *   440     + Enter  -> frequentie 440 Hz         (FREQ_INPUT_FREQ,   waarde in centi-Hz)
 *   261.63  + Enter  -> frequentie 261,63 Hz
 *   v50     + Enter  -> volume 50 %, v0 .. v100   (FREQ_INPUT_VOLUME, waarde in procent)
 *
 * Leest via stdin, dus het werkt op de console-poort die ESP-IDF gebruikt
 * (UART- of USB-poort van het board, wat je in idf.py monitor open hebt).
 */
#ifndef FREQ_INPUT_H
#define FREQ_INPUT_H

#include <stdint.h>
#include <stdio.h>

#define FREQ_INPUT_ECHO          1          // getypte tekens terugsturen
#define FREQ_INPUT_DEBUG         0          // 1 = elk ontvangen byte printen

#define FREQ_INPUT_MIN_CENTIHZ   10UL       // 0,10 Hz
#define FREQ_INPUT_MAX_CENTIHZ   4800000UL  // 48 kHz = Nyquist bij 96 kHz samplefrequentie

#define FREQ_INPUT_NIETS         0          // nog geen complete regel
#define FREQ_INPUT_FREQ          1          // *waarde = frequentie in centi-Hz
#define FREQ_INPUT_VOLUME        2          // *waarde = volume in procent (0..100)

// Eén teken uit stdin, of FREQ_RX_GEEN als er niks is (ESP-IDF leest standaard niet-blokkerend)
#define FREQ_RX_GEEN             0x100u
static inline uint16_t freq_rx_lees(void)
{
    int c = getchar();
    if (c == EOF) {
        clearerr(stdin);                    // anders blijft getchar EOF geven
        return FREQ_RX_GEEN;
    }
    return (uint16_t)(uint8_t)c;
}
#define FREQ_RX_LEES()           freq_rx_lees()

uint8_t freq_input_poll(uint32_t *waarde);
void    freq_input_debug(void);

#endif // FREQ_INPUT_H
