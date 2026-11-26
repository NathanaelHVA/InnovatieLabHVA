/*!
 * \file    pinnen.h
 * \brief   ALLE pinnen van de VCO-synth op één plek (ESP32-S3 N16R8)
 *
 * Niet gebruiken:
 *   GPIO19/20  USB          GPIO43/44  terminal (UART0)          GPIO35/36/37  PSRAM
 *   GPIO45     strapping (flashspanning)
 */
#ifndef PINNEN_H
#define PINNEN_H

/* ---------- 2x DAC80501 (SPI via dedicated GPIO, core 1) ---------- */
#define DAC_PIN_SCLK     48      // SCLK van DAC 1 en DAC 2
#define DAC_PIN_SDIN1    47      // SDIN van DAC 1
#define DAC_PIN_SDIN2     4      // SDIN van DAC 2 (was GPIO0, de BOOT-pin)
#define DAC_PIN_SYNC     21      // SYNC van DAC 1 en DAC 2
//                                  VDD -> 3V3, GND -> GND, SPI2C -> GND

/* ---------- gate voor de envelope generator ---------- */
#define GATE_PIN          1      // hoog zolang er een toets is ingedrukt (zie GATE_VAN_TOETSEN in main.c)
#define GATE_INV_PIN      2      // omgekeerde gate: laag zolang er een toets is ingedrukt

/* ---------- TFT ST7735 (128x160) ---------- */
#define TFT_PIN_SCK      42      // SCK
#define TFT_PIN_SDA      41      // SDA (= MOSI)
#define TFT_PIN_CS       40      // CS
#define TFT_PIN_A0       39      // A0 (= DC, data/command)
#define TFT_PIN_RESET    38      // RESET
//                                  LED -> 3V3, VCC -> 3V3, GND -> GND

/* ---------- toetsen (naar 3V3, interne pull-down: ingedrukt = 1) ---------- */
#define TOETS0_GPIO      14      // C
#define TOETS1_GPIO      13      // C#
#define TOETS2_GPIO      12      // D
#define TOETS3_GPIO      11      // D#
#define TOETS4_GPIO      10      // E
#define TOETS5_GPIO       9      // F
#define TOETS6_GPIO      46      // F#  (strapping-pin: niet indrukken tijdens opstarten)
#define TOETS7_GPIO       3      // G   (strapping-pin: niet indrukken tijdens opstarten)
#define TOETS8_GPIO       8      // G#
#define TOETS9_GPIO      18      // A
#define TOETS10_GPIO     17      // A#
#define TOETS11_GPIO     16      // B

/* ---------- overige knoppen (ook naar 3V3) ---------- */
#define OCTAAF_GPIO      15      // octaafknop
#define GOLF_VCO1_GPIO    7      // golfvorm VCO 1 wisselen
#define GOLF_VCO2_GPIO    6      // golfvorm VCO 2 wisselen

/* ---------- potmeter ---------- */
#define POT_GPIO          5      // loper; uiteinden naar 3V3 en GND (GPIO5 = ADC1 kanaal 4)
                                 // verdeelt het volume: VCO 1 = x %, VCO 2 = 100 - x %

#endif /* PINNEN_H */
