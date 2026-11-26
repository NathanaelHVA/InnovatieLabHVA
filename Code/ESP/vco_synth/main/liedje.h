/*!
 * \file    liedje.h
 * \brief   Liedjes afspelen op de synth: Vader Jakob en He's a Pirate
 *
 * Gebruik (vanuit main.c, elke TIK_MS):
 *   liedje_start(LIEDJE_PIRAAT);                      // één keer, om te beginnen
 *   uint32_t f;
 *   if (liedje_tik(TIK_MS, &f)) setFreq_centihz(f);   // nieuwe noot: frequentie zetten
 *   gate = liedje_gate();                             // 1 = noot klinkt, 0 = even stil
 */
#ifndef LIEDJE_H_
#define LIEDJE_H_

#include <stdint.h>

typedef enum {
    LIEDJE_GEEN = -1,           // geen liedje (liedje_start doet dan niks)
    LIEDJE_VADER_JAKOB,
    LIEDJE_PIRAAT,              // He's a Pirate (Pirates of the Caribbean)
    LIEDJE_AANTAL
} liedje_nr_t;

void        liedje_start(liedje_nr_t nr);               // begint (opnieuw) bij de eerste noot
void        liedje_stop(void);
int         liedje_bezig(void);                         // 1 = speelt nu
liedje_nr_t liedje_huidig(void);                        // welk liedje gekozen is (ook na stoppen)
const char *liedje_naam(void);                          // naam van dat liedje, voor in de terminal

// Elke tik aanroepen. Geeft 1 als er een nieuwe noot begint; de frequentie staat dan in *f_centi.
int  liedje_tik(uint32_t tik_ms, uint32_t *f_centi);

// Gate voor de envelope generator: hoog tijdens een noot, kort laag ertussen (zodat hij opnieuw start)
int  liedje_gate(void);

#endif /* LIEDJE_H_ */
