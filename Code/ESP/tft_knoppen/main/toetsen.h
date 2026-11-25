#ifndef TOETSEN_H_
#define TOETSEN_H_

#include <stdint.h>

#define AANTAL_TOETSEN  12      // toets 0..11, pinnen: zie pinnen.h
#define OCTAAF_MAX      4       // octaaf loopt 0,1,2,3,4,0,1,...

// Wat er sinds de vorige poll nieuw is ingedrukt
typedef struct {
    uint16_t toetsen;           // bit i = toets i zojuist ingedrukt
    uint8_t  octaaf;            // 1 = octaafknop zojuist ingedrukt
    uint8_t  golf;              // bit 0 = golfknop VCO 1, bit 1 = golfknop VCO 2 zojuist ingedrukt
    uint8_t  reserve;           // 1 = reserveknop zojuist ingedrukt
} toetsen_event_t;

void            toetsen_init(void);
toetsen_event_t toetsen_poll(void);         // aanroepen elke ~10 ms (debounce)
uint8_t         toetsen_octaaf(void);       // huidige octaaf (0..OCTAAF_MAX)
int             toetsen_gpio(uint8_t toets);    // GPIO-nummer van toets 0..11

#endif /* TOETSEN_H_ */
