/*!
 * \file    dac80501.c
 * \brief   Twee DAC80501's via dedicated GPIO (zie dac80501.h)
 */

#include "dac80501.h"
#include "driver/gpio.h"
#include "driver/dedic_gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "hal/dedic_gpio_cpu_ll.h"     // dedic_gpio_cpu_ll_write_mask: één CPU-instructie per schrijfactie

// Bitnummers binnen de bundel: volgorde van pinnen[] hieronder
#define B_SCLK   (1u << 0)
#define B_SDIN1  (1u << 1)
#define B_SDIN2  (1u << 2)
#define B_SYNC   (1u << 3)
#define B_ALLE   (B_SCLK | B_SDIN1 | B_SDIN2 | B_SYNC)

// SCLK niet sneller maken dan nodig (DAC80501 kan 50 MHz, maar draadjes op een breadboard niet):
// zoveel keer een "nop" na elke flank. 4 geeft ongeveer 10 MHz SCLK bij 240 MHz CPU.
#define DAC_WACHT   4

static dedic_gpio_bundle_handle_t bus;

// Kanaalbits zoals de CPU ze ziet (de bundel kan op een willekeurige plek in het register zitten)
static uint32_t k_alle, k_sclk, k_sdin1, k_sdin2, k_sync;

static inline __attribute__((always_inline)) void wacht(void)
{
    for (int i = 0; i < DAC_WACHT; i++) __asm__ __volatile__("nop");
}

void dac80501_init(void)
{
    static const int pinnen[4] = { DAC_PIN_SCLK, DAC_PIN_SDIN1, DAC_PIN_SDIN2, DAC_PIN_SYNC };

    // Eerst gewone uitgangen van maken (zo vraagt de dedicated-GPIO-driver het), SYNC hoog
    gpio_set_level(DAC_PIN_SYNC, 1);
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << DAC_PIN_SCLK) | (1ULL << DAC_PIN_SDIN1) |
                        (1ULL << DAC_PIN_SDIN2) | (1ULL << DAC_PIN_SYNC),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    // Dan aan de CPU koppelen
    dedic_gpio_bundle_config_t cfg = {
        .gpio_array = pinnen,
        .array_size = 4,
        .flags = { .out_en = 1 },
    };
    ESP_ERROR_CHECK(dedic_gpio_new_bundle(&cfg, &bus));

    // Welke CPU-kanalen heeft de bundel gekregen? Bit 0 van de bundel = laagste bit van het masker.
    uint32_t masker;
    ESP_ERROR_CHECK(dedic_gpio_get_out_mask(bus, &masker));
    uint32_t off = (uint32_t)__builtin_ctz(masker);
    k_alle  = masker;
    k_sclk  = B_SCLK  << off;
    k_sdin1 = B_SDIN1 << off;
    k_sdin2 = B_SDIN2 << off;
    k_sync  = B_SYNC  << off;

    dedic_gpio_cpu_ll_write_mask(k_alle, k_sync);       // rust: SYNC hoog, SCLK laag

    esp_rom_delay_us(1000);                             // de DAC heeft 250 us POR-tijd nodig

    dac80501_write_reg2(DAC_REG_GAIN, DAC_GAIN_WAARDE, DAC_GAIN_WAARDE);
    esp_rom_delay_us(1);
    dac80501_write2(0x8000, 0x8000);                    // midscale = stilte
}

// In IRAM: snel, en geen last van flash-cache-missers tijdens het versturen
void IRAM_ATTR dac80501_write_reg2(uint8_t reg, uint16_t data1, uint16_t data2)
{
    uint32_t w1 = ((uint32_t)reg << 16) | data1;
    uint32_t w2 = ((uint32_t)reg << 16) | data2;

    const uint32_t alle = k_alle, sclk = k_sclk, s1 = k_sdin1, s2 = k_sdin2;   // in registers houden

    dedic_gpio_cpu_ll_write_mask(alle, 0);              // SYNC laag: frame begint
    wacht();
    for (int b = 23; b >= 0; b--) {
        uint32_t d = (((w1 >> b) & 1u) ? s1 : 0) |
                     (((w2 >> b) & 1u) ? s2 : 0);
        dedic_gpio_cpu_ll_write_mask(alle, d | sclk);   // SCLK omhoog, data klaarzetten
        wacht();
        dedic_gpio_cpu_ll_write_mask(alle, d);          // SCLK omlaag: DAC leest het bit
        wacht();
    }
    dedic_gpio_cpu_ll_write_mask(alle, k_sync);         // SYNC hoog: beide DAC's werken bij
}