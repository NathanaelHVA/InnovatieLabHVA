/*!
 * \file    freq_input.h
 * \brief   Frequentie intypen via de seriële terminal
 *
 * Typ in je terminal bijvoorbeeld   440  of  261.63  (punt of komma) gevolgd door Enter.
 * Geldig: 0,01 ... 20000 Hz, hoogstens 2 decimalen (meer decimalen worden genegeerd).
 * Backspace wist wat je tot nu toe getypt hebt. Andere tekens worden genegeerd.
 *
 * De tekens komen uit de ontvangstbuffer van serialF0 (uartF0_getc(), wacht niet).
 * Roep freq_input_poll() regelmatig aan, bijvoorbeeld elke ms; de buffer is 100 tekens groot.
 */
#ifndef FREQ_INPUT_H
#define FREQ_INPUT_H

#include <stdint.h>
#include "serialF0.h"

/* ---------- instellingen ---------- */
#ifndef FREQ_INPUT_ECHO
#define FREQ_INPUT_ECHO         1           // 1 = ingetypte tekens terugsturen; 0 als je terminal zelf echo't
#endif
#ifndef FREQ_INPUT_DEBUG
#define FREQ_INPUT_DEBUG        1           // 1 = elke ontvangen byte als [RX xx] tonen + teller; 0 als alles werkt
#endif
#ifndef FREQ_INPUT_MIN_CENTIHZ
#define FREQ_INPUT_MIN_CENTIHZ  1UL         // 0,01 Hz
#endif
#ifndef FREQ_INPUT_MAX_CENTIHZ
#define FREQ_INPUT_MAX_CENTIHZ  2000000UL   // 20 kHz (Nyquist is bij FS = 50 kHz 25 kHz)
#endif

/* Een teken lezen: geeft 0..255, of FREQ_RX_GEEN als er niets is. Vervangbaar (de test op de pc doet dat). */
#ifndef FREQ_RX_LEES
#define FREQ_RX_LEES()    uartF0_getc()
#define FREQ_RX_GEEN      UART_NO_DATA
#endif

/*! \brief Print "fi-v3 rx=<aantal ontvangen bytes>" (alleen als FREQ_INPUT_DEBUG 1 is) */
void freq_input_debug(void);

/*! \brief Ontvangen tekens verwerken.
 *  \param f_centihz  hier komt de ingetypte frequentie in centi-Hz (440 Hz = 44000)
 *  \return 1 als er net een complete, geldige frequentie is ingevoerd, anders 0 */
uint8_t freq_input_poll(uint32_t *f_centihz);

#endif // FREQ_INPUT_H