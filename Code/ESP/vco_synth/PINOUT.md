PINOUT VCO-SYNTH (ESP32-S3 N16R8)
Alle pinnen staan ook in main/pinnen.h
 
 
DAC'S (2x DAC80501)
 
GPIO48   SCLK, naar DAC 1 en DAC 2
GPIO47   SDIN DAC 1
GPIO0    SDIN DAC 2, niet op BOOT drukken tijdens het spelen
GPIO21   SYNC, naar DAC 1 en DAC 2
3V3      VDD van beide DAC's
GND      GND en SPI2C van beide DAC's
 
 
GATE
 
GPIO4    gate naar de envelope generator, hoog zolang er een toets is ingedrukt
 
 
SCHERM (ST7735)
 
GPIO42   SCK
GPIO41   SDA
GPIO40   CS
GPIO39   A0
GPIO38   RESET
3V3      VCC en LED
GND      GND
 
 
TOETSEN
 
GPIO14   toets 0   C
GPIO13   toets 1   C#
GPIO12   toets 2   D
GPIO11   toets 3   D#
GPIO10   toets 4   E
GPIO9    toets 5   F
GPIO46   toets 6   F#   niet indrukken tijdens het opstarten
GPIO3    toets 7   G    niet indrukken tijdens het opstarten
GPIO8    toets 8   G#
GPIO18   toets 9   A
GPIO17   toets 10  A#
GPIO16   toets 11  B
 
 
KNOPPEN
 
GPIO15   octaaf
GPIO7    golfvorm VCO 1
GPIO6    golfvorm VCO 2
GPIO1    reserve
 
 
POTMETER
 
GPIO5    loper, de uiteinden naar 3V3 en GND
         verdeelt het volume tussen VCO 1 en VCO 2
 