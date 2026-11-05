#ifndef FREQUENTIE_H_
#define FREQUENTIE_H_

#include <stdint.h>

#define START_FREQUENCY 130.8   // frequentie van toets 0 bij octaaf 0

// Frequentie in honderdsten Hz (dus 26160 = 261,60 Hz)
// f = START_FREQUENCY * 2^octaaf * 2^(toets/12)
uint32_t frequentie_centihz(uint8_t toets, uint8_t octaaf);

#endif /* FREQUENTIE_H_ */