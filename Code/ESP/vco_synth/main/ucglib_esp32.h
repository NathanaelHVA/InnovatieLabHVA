#ifndef UCGLIB_ESP32_H
#define UCGLIB_ESP32_H

#include "ucg.h"

// Maximale SPI-klok naar het display (ST7735 kan ~15 MHz; 10 MHz is veilig op een breadboard)
#define TFT_SPI_MAX_HZ   10000000

// Communicatie-callback voor ucglib: hardware-SPI (SPI2) + GPIO voor CS, A0 en RESET
int16_t ucg_com_esp32_cb(ucg_t *ucg, int16_t msg, uint16_t arg, uint8_t *data);

#endif /* UCGLIB_ESP32_H */
