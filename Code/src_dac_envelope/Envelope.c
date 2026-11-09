/*!
 * \file    Envelope.c
 * \brief   Gate voor de 555-envelope generator (zie envelope.h)
 */

#include "envelope.h"

static uint8_t gate_aan = 0;

void envelope_init(void)
{
    // Rusttoestand: gate laag (RST actief, 555 in rust), !gate hoog (TRIG blijft hoog via R14).
    // Eerst het niveau zetten en dan pas uitgang: geen glitch op de pinnen.
    ENV_PORT.OUTCLR = ENV_GATE_bm;
    ENV_PORT.OUTSET = ENV_NGATE_bm;
    ENV_PORT.DIRSET = ENV_GATE_bm | ENV_NGATE_bm;
    gate_aan = 0;
}

void envelope_gate(uint8_t aan)
{
    aan = aan ? 1 : 0;
    if (aan == gate_aan) return;        // geen flanken als er niets verandert: geen hertrigger
    gate_aan = aan;

    if (aan) {
        ENV_PORT.OUTSET = ENV_GATE_bm;  // eerst RST vrijgeven ...
        ENV_PORT.OUTCLR = ENV_NGATE_bm; // ... dan de dalende flank: C3 trekt TRIG kort laag = attack
    } else {
        ENV_PORT.OUTCLR = ENV_GATE_bm;  // RST laag: attack stopt, release begint
        ENV_PORT.OUTSET = ENV_NGATE_bm; // C3 laadt terug, D4 klemt de positieve piek af
    }
}
