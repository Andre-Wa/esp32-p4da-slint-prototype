/**
 * wifi_init.c
 *
 * Bring-up mínimo do WiFi via ESP32-C6 (coprocessador, conectado por
 * SDIO) usando o componente espressif/esp_hosted + espressif/esp_wifi_remote.
 * Depois desse bring-up, o resto do app usa a API esp_wifi PADRÃO — o
 * esp_wifi_remote redireciona tudo pro C6 de forma transparente.
 *
 * ATENÇÃO — antes de confiar que WiFi está "funcionando de verdade":
 * essa placa pode sair de fábrica com o firmware do C6 desatualizado
 * (2.3.0), incompatível com host moderno (espera ~2.12+). Sintoma: WiFi
 * associa e cai em loop. Ver README, seção WiFi/C6, se isso acontecer.
 *
 * Nomes de struct/função confirmados lendo os headers reais do
 * componente (managed_components/espressif__esp_hosted/host/compat/
 * include/esp_hosted_transport_config.h e .../port/os/include/
 * eh_host_transport_config.h) — a primeira tentativa tinha nomes de
 * campo chutados (documentação do componente é escassa, ainda em
 * pre-release) e não compilava.
 */

#include "wifi_init.h"
#include "board_config.h"

#include "esp_hosted_transport_config.h"  // aliases esp_hosted_* -> eh_host_*
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "wifi_init";

static esp_err_t configure_sdio_transport(void)
{
    /* struct esp_hosted_sdio_config é um #define pra struct
     * eh_host_sdio_config (ver esp_hosted_transport_config.h). Os
     * campos pin_* são do tipo eh_gpio_pin_t ({ port, pin }), não um
     * inteiro puro — por isso .pin_clk.pin, não .pin_clk. */
    struct esp_hosted_sdio_config transport_config = INIT_DEFAULT_HOST_SDIO_CONFIG();

    transport_config.clock_freq_khz = BOARD_C6_SDIO_FREQ_KHZ;
    transport_config.pin_clk.pin    = BOARD_C6_SDIO_CLK_GPIO;
    transport_config.pin_cmd.pin    = BOARD_C6_SDIO_CMD_GPIO;
    transport_config.pin_d0.pin     = BOARD_C6_SDIO_D0_GPIO;
    transport_config.pin_d1.pin     = BOARD_C6_SDIO_D1_GPIO;
    transport_config.pin_d2.pin     = BOARD_C6_SDIO_D2_GPIO;
    transport_config.pin_d3.pin     = BOARD_C6_SDIO_D3_GPIO;
    transport_config.pin_reset.pin  = BOARD_C6_RESET_GPIO;
    /* .port fica no valor default (não usado no ESP-IDF — é uma
     * abstração multi-plataforma do componente). bus_width, slot,
     * rx_mode, block_mode, iomux_enable e os tamanhos de fila ficam
     * como o default já veio de INIT_DEFAULT_HOST_SDIO_CONFIG(). */

    esp_hosted_transport_err_t err = esp_hosted_sdio_set_config(&transport_config);
    if (err != ESP_TRANSPORT_OK) {
        ESP_LOGE(TAG, "esp_hosted_sdio_set_config falhou (código %d)", (int)err);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t board_wifi_init(void)
{
    ESP_RETURN_ON_ERROR(configure_sdio_transport(), TAG, "configurar transporte SDIO");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs_flash_init");

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "esp_event_loop_create_default");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init falhou: %s (o link SDIO com o C6 pode não estar respondendo)",
                 esp_err_to_name(err));
        return err;
    }

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "esp_wifi_set_mode");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start");

    ESP_LOGI(TAG, "WiFi (via ESP32-C6) inicializado");
    return ESP_OK;
}

void board_wifi_scan_and_log(void)
{
    ESP_LOGI(TAG, "iniciando scan de redes WiFi...");
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
    };

    esp_err_t err = esp_wifi_scan_start(&scan_config, true); // bloqueante
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan falhou: %s", esp_err_to_name(err));
        return;
    }

    uint16_t num_networks = 0;
    esp_wifi_scan_get_ap_num(&num_networks);
    ESP_LOGI(TAG, "%d rede(s) encontrada(s)", num_networks);

    if (num_networks == 0) {
        return;
    }

    wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * num_networks);
    if (!ap_records) {
        ESP_LOGE(TAG, "sem memória pra lista de redes");
        return;
    }

    esp_wifi_scan_get_ap_records(&num_networks, ap_records);
    for (int i = 0; i < num_networks; i++) {
        ESP_LOGI(TAG, "  [%d] SSID=\"%s\" RSSI=%d canal=%d",
                 i, (char *)ap_records[i].ssid, ap_records[i].rssi, ap_records[i].primary);
    }

    free(ap_records);
}
