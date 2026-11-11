/*!
 * \file    envelope.h
 * \brief   Gate voor de 555-envelope generator, op twee GPIO-pinnen
 *
 * De schakeling heeft de gate in twee polariteiten nodig:
 *   GATE  (PD0)  hoog zolang er een toets ingedrukt is  -> RST (pin 4) en het release-pad (R5)
 *   NGATE (PD1)  omgekeerd van GATE                     -> C3 -> TRIG (pin 2)
 * De dalende flank van NGATE geeft via C3 de korte triggerpuls op TRIG.
 *
 * De pinnen kun je hieronder aanpassen.
 */
#ifndef ENVELOPE_H
#define ENVELOPE_H

#include <avr/io.h>
#include <stdint.h>

#define ENV_PORT      PORTD
#define ENV_GATE_bm   PIN0_bm       // PD0
#define ENV_NGATE_bm  PIN1_bm       // PD1

/*! \brief Pinnen als uitgang, rusttoestand: gate laag, !gate hoog */
void envelope_init(void);

/*! \brief Gate aan (1) of uit (0). Doet alleen iets als de toestand verandert. */
void envelope_gate(uint8_t aan);

#endif // ENVELOPE_H
