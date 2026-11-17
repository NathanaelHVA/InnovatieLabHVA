#include <avr/io.h>
#include "adc.h"

#define MIN_VALUE 11        // laagste gemeten res is -11
#define MAX_VALUE 2047      // hoogste res

void adc_init(void)
{
    PORTB.DIRCLR = PIN7_bm;                        // PB7 als ingang

    ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc |     // PB7 naar +
                       ADC_CH_MUXNEG_GND_MODE3_gc; // GND naar -
    ADCB.CH0.CTRL    = ADC_CH_INPUTMODE_DIFF_gc;   // differentieel
    ADCB.REFCTRL     = ADC_REFSEL_INTVCC_gc;       // referentie = VCC/1,6
    ADCB.CTRLB       = ADC_RESOLUTION_12BIT_gc |
                       ADC_CONMODE_bm;             // signed conversie
    ADCB.PRESCALER   = ADC_PRESCALER_DIV16_gc;
    ADCB.CTRLA       = ADC_ENABLE_bm;
}

int16_t adc_lees(void)
{
    int16_t res;

    ADCB.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCB.CH0.INTFLAGS & ADC_CH_CHIF_bm));
    res = ADCB.CH0.RES;
    ADCB.CH0.INTFLAGS |= ADC_CH_CHIF_bm;

    return res;
}

adc_meting_t adc_meet(void)
{
    adc_meting_t m;
    int16_t p;

    m.res = adc_lees();

    // ruw percentage: -11 = 0%, 2047 = 100%
    p = ((int32_t)(m.res + MIN_VALUE) * 100) / (MAX_VALUE + MIN_VALUE);
    if (p < 0)   p = 0;
    if (p > 100) p = 100;

    // afronden op 5-tallen: 0, 5, 10, ... 100
    m.procent  = ((p + 2) / 5) * 5;
    m.procent2 = 100 - m.procent;

    return m;
}