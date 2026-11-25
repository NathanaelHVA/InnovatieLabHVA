# Pinout TFT + knoppen (ESP32-S3 N16R8)

| Functie              | GPIO | Aansluiten op                         |
|----------------------|------|---------------------------------------|
| TFT SCK              | 42   | SCK van het display                   |
| TFT SDA              | 41   | SDA van het display                   |
| TFT CS               | 40   | CS van het display                    |
| TFT A0               | 39   | A0 van het display                    |
| TFT RESET            | 38   | RESET van het display                 |
| TFT VCC / LED / GND  | -    | 3V3 / 3V3 / GND                       |
| Toets 0  (C)         | 14   | knop naar 3V3                         |
| Toets 1  (C#)        | 13   | knop naar 3V3                         |
| Toets 2  (D)         | 12   | knop naar 3V3                         |
| Toets 3  (D#)        | 11   | knop naar 3V3                         |
| Toets 4  (E)         | 10   | knop naar 3V3                         |
| Toets 5  (F)         | 9    | knop naar 3V3                         |
| Toets 6  (F#)        | 46   | knop naar 3V3 (niet indrukken bij opstarten) |
| Toets 7  (G)         | 3    | knop naar 3V3 (niet indrukken bij opstarten) |
| Toets 8  (G#)        | 8    | knop naar 3V3                         |
| Toets 9  (A)         | 18   | knop naar 3V3                         |
| Toets 10 (A#)        | 17   | knop naar 3V3                         |
| Toets 11 (B)         | 16   | knop naar 3V3                         |
| Octaafknop           | 15   | knop naar 3V3                         |
| Golfvorm VCO 1       | 7    | knop naar 3V3                         |
| Golfvorm VCO 2       | 6    | knop naar 3V3                         |
| Reserveknop          | 5    | knop naar 3V3 (doet nog niks)         |
| Volumepotmeter       | 1    | loper; uiteinden naar 3V3 en GND      |

Knoppen: interne pull-down staat aan, dus geen weerstanden nodig (ingedrukt = 3V3).
Golfvormknop: sinus -> blok -> driehoek -> zaagtand, het gekozen rondje op het scherm wordt groen.
Alle pinnen staan in main/pinnen.h.
