#include <math.h>
#include "frequentie.h"

uint32_t frequentie_centihz(uint8_t toets, uint8_t octaaf)
{
    double x = START_FREQUENCY * pow(2.0, (double)octaaf);
    double f = x * pow(2.0, (double)toets / 12.0);

    return (uint32_t)(f * 100.0 + 0.5);
}