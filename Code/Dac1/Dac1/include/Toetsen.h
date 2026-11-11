#ifndef TOETSEN_H_
#define TOETSEN_H_

#include <stdint.h>

#define AANTAL_TOETSEN  12      // toets 0..7 = PA0..PA7, toets 8, 9, 11 = PB0, PB1, PB3, toets 10 = PC0
#define OCTAAF_MAX      4       // octaaf loopt 0,1,2,3,4,0,1,...

// Wat er sinds de vorige poll nieuw is ingedrukt
typedef struct {
    uint16_t toetsen;           // bit i = toets i zojuist ingedrukt
    uint8_t  octaaf;            // 1 = octaafknop zojuist ingedrukt
} toetsen_event_t;

void            toetsen_init(void);
toetsen_event_t toetsen_poll(void);         // aanroepen elke ~10 ms (debounce)
uint8_t         toetsen_octaaf(void);       // huidige octaaf (0..OCTAAF_MAX)
char            toetsen_poort(uint8_t toets);   // 'A', 'B' of 'C'
uint8_t         toetsen_pin(uint8_t toets);     // pinnummer binnen de poort

#endif /* TOETSEN_H_ */
