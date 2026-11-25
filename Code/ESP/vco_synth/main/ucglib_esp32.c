/*!
 * \file    ucglib_esp32.c
 * \brief   ucglib-communicatie voor de ESP32-S3 (vervangt ucglib_xmega.c + spi.c)
 *
 * SCK en SDA gaan via de SPI-hardware (SPI2), CS, A0 en RESET zijn gewone GPIO's
 * die ucglib zelf aan- en uitzet, net als op de XMEGA. Pinnen: zie pinnen.h.
 */
#include "ucglib_esp32.h"
#include "pinnen.h"

#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_rom_sys.h"

static spi_device_handle_t tft = NULL;

// Buffer om herhaalde pixels in één keer te versturen. 192 = 96 x 2 = 64 x 3 bytes.
static DMA_ATTR uint8_t herhaal_buf[192];

static void spi_stuur(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;                                    // in bits
    if (len <= 4) {                                        // kleine stukjes: direct in de transactie
        t.flags = SPI_TRANS_USE_TXDATA;
        memcpy(t.tx_data, data, len);
    } else {
        t.tx_buffer = data;
    }
    ESP_ERROR_CHECK(spi_device_polling_transmit(tft, &t));
}

// "aantal" keer hetzelfde patroon van n bytes (1, 2 of 3) versturen, in blokken
static void spi_herhaal(const uint8_t *patroon, uint8_t n, uint16_t aantal)
{
    uint16_t per_blok = sizeof(herhaal_buf) / n;
    for (uint16_t i = 0; i < per_blok; i++)
        memcpy(&herhaal_buf[i * n], patroon, n);

    while (aantal > 0) {
        uint16_t deze = (aantal > per_blok) ? per_blok : aantal;
        spi_stuur(herhaal_buf, (size_t)deze * n);
        aantal -= deze;
    }
}

static void tft_init(uint16_t sclk_periode_ns)
{
    if (tft != NULL) return;                               // al gedaan (POWER_UP kan vaker komen)

    // CS hoog (niet geselecteerd), A0 en RESET laag, allemaal uitgang
    gpio_set_level(TFT_PIN_CS, 1);
    gpio_set_level(TFT_PIN_A0, 0);
    gpio_set_level(TFT_PIN_RESET, 0);
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << TFT_PIN_CS) | (1ULL << TFT_PIN_A0) | (1ULL << TFT_PIN_RESET),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    spi_bus_config_t bus = {
        .mosi_io_num     = TFT_PIN_SDA,
        .miso_io_num     = -1,                             // display stuurt niks terug
        .sclk_io_num     = TFT_PIN_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = sizeof(herhaal_buf),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    // Klok: wat ucglib vraagt, maar nooit sneller dan TFT_SPI_MAX_HZ
    int hz = TFT_SPI_MAX_HZ;
    if (sclk_periode_ns > 0 && 1000000000 / sclk_periode_ns < hz)
        hz = 1000000000 / sclk_periode_ns;

    spi_device_interface_config_t dev = {
        .mode           = 0,                               // SPI mode 0, zoals op de XMEGA
        .clock_speed_hz = hz,
        .spics_io_num   = -1,                              // CS doet ucglib zelf
        .queue_size     = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &tft));
}

int16_t ucg_com_esp32_cb(ucg_t *ucg, int16_t msg, uint16_t arg, uint8_t *data)
{
    (void)ucg;

    switch (msg) {
    case UCG_COM_MSG_POWER_UP:
        tft_init(((ucg_com_info_t *)data)->serial_clk_speed);
        break;

    case UCG_COM_MSG_POWER_DOWN:
        break;

    case UCG_COM_MSG_DELAY:                                // arg = aantal microseconden
        esp_rom_delay_us(arg);
        break;

    case UCG_COM_MSG_CHANGE_RESET_LINE:
        gpio_set_level(TFT_PIN_RESET, arg ? 1 : 0);
        break;

    case UCG_COM_MSG_CHANGE_CD_LINE:
        gpio_set_level(TFT_PIN_A0, arg ? 1 : 0);
        break;

    case UCG_COM_MSG_CHANGE_CS_LINE:
        gpio_set_level(TFT_PIN_CS, arg ? 1 : 0);
        break;

    case UCG_COM_MSG_SEND_BYTE: {
        uint8_t b = (uint8_t)arg;
        spi_stuur(&b, 1);
        break;
    }

    case UCG_COM_MSG_REPEAT_1_BYTE:
        spi_herhaal(data, 1, arg);
        break;

    case UCG_COM_MSG_REPEAT_2_BYTES:
        spi_herhaal(data, 2, arg);
        break;

    case UCG_COM_MSG_REPEAT_3_BYTES:
        spi_herhaal(data, 3, arg);
        break;

    case UCG_COM_MSG_SEND_STR:                             // arg bytes uit data
        spi_stuur(data, arg);
        break;

    case UCG_COM_MSG_SEND_CD_DATA_SEQUENCE:
        // paren van (A0-stand, byte): 0 = A0 laten staan, 1 = A0 laag, anders A0 hoog
        while (arg > 0) {
            if (*data != 0)
                gpio_set_level(TFT_PIN_A0, (*data == 1) ? 0 : 1);
            data++;
            spi_stuur(data, 1);
            data++;
            arg--;
        }
        break;

    default:
        return 1;
    }
    return 1;
}
