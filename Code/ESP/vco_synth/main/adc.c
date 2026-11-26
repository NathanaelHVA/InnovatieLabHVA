#include "adc.h"
#include "pinnen.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

// Bereik van de potmeter in ruwe ADC-waarden. Met 12 dB demping meet de ADC tot ~3,1 V,
// dus het laatste stukje van de potmeter (3,1..3,3 V) geeft al 4095.
// Haalt jouw potmeter 0 of 100 % niet helemaal: pas MIN_VALUE / MAX_VALUE aan.
#define MIN_VALUE   0
#define MAX_VALUE   4095

static adc_oneshot_unit_handle_t adc;
static adc_channel_t             kanaal;

void adc_init(void)
{
    adc_unit_t unit;
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(POT_GPIO, &unit, &kanaal));   // GPIO1 -> ADC1 kanaal 0

    adc_oneshot_unit_init_cfg_t ucfg = { .unit_id = unit };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&ucfg, &adc));

    adc_oneshot_chan_cfg_t ccfg = {
        .atten    = ADC_ATTEN_DB_12,       // bereik 0 .. ~3,1 V
        .bitwidth = ADC_BITWIDTH_12,       // 0 .. 4095
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, kanaal, &ccfg));
}

int16_t adc_lees(void)
{
    int raw = 0;
    adc_oneshot_read(adc, kanaal, &raw);
    return (int16_t)raw;
}

adc_meting_t adc_meet(void)
{
    adc_meting_t m;
    int32_t p;

    m.res = adc_lees();

    // ruw percentage: MIN_VALUE = 0 %, MAX_VALUE = 100 %
    p = ((int32_t)(m.res - MIN_VALUE) * 100) / (MAX_VALUE - MIN_VALUE);
    if (p < 0)   p = 0;
    if (p > 100) p = 100;

    // afronden op 10-tallen: 0, 10, 20, ... 100
    m.procent  = (int16_t)(((p + 5) / 10) * 10);
    m.procent2 = (int16_t)(100 - m.procent);

    return m;
}
