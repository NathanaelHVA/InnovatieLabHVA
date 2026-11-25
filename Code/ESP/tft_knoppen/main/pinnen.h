/*!
 * \file    pinnen.h
 * \brief   Alle pinnen van het TFT+knoppen-project op één plek (ESP32-S3 N16R8)
 *
 * De knoppen zitten op volgorde langs de linkerrij van het board (GPIO14 ... GPIO5).
 * Geen botsing met de signaalgenerator: die stuurt GPIO2, 4, 21, 45, 47, 48 aan
 * en leest dezelfde golfvormknoppen (GPIO7 en 6).
 *
 * Niet gebruikt (en niet gebruiken):
 *   GPIO0  BOOT-knop          GPIO19/20  USB          GPIO43/44  terminal (UART0)
 *   GPIO35/36/37  PSRAM
 */
#ifndef PINNEN_H
#define PINNEN_H

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
#define RESERVE_GPIO      1      // reserveknop (doet nog niks, print alleen een melding)

/* ---------- volumepotmeter ---------- */
#define POT_GPIO          5      // loper; uiteinden naar 3V3 en GND (GPIO1 = ADC1 kanaal 0)

#endif /* PINNEN_H */
