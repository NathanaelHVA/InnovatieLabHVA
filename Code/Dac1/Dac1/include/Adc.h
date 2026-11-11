#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

/* Resultaat van één meting */
typedef struct {
    int16_t res;        // ruwe ADC-waarde
    int16_t procent;    // percentage, afgerond op 5-tallen (0..100)
    int16_t procent2;   // de rest: 100 - procent
} adc_meting_t;

void         adc_init(void);   // ADCB, kanaal 0, PB7 (differentieel, signed)
int16_t      adc_lees(void);   // één ruwe conversie
adc_meting_t adc_meet(void);   // meting + omrekening naar procenten

#endif /* ADC_H_ */