# Pinout signaalgenerator (ESP32-S3 N16R8)

| Functie              | GPIO | Aansluiten op                         |
|----------------------|------|---------------------------------------|
| DAC SCLK             | 48   | SCLK van DAC 1 en DAC 2               |
| DAC SDIN 1           | 47   | SDIN van DAC 1 12                       |
| DAC SDIN 2           | 45   | SDIN van DAC 2 (geen pull-up!)        |
| DAC SYNC             | 21   | SYNC van DAC 1 en DAC 2 (pull-up)     |
| Gate                 | 4    | gate-ingang envelope generator        |
| Gate geinverteerd    | 5    | geinverteerde gate                    |
| DAC VDD / GND        | -    | 3V3 / GND                             |

SPI2C-pin van beide DAC's naar GND (SPI-mode).
GPIO45 is een strapping-pin: moet laag zijn bij opstarten, dus nooit een pull-up op SDIN 2.
Zit de RGB-LED van het board op GPIO47 of 48, dan knippert die mee (onschuldig).
Pinnen staan in main/dac80501.h (DAC) en main/main.c (gate).
