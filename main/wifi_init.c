#include "wifi_init.h"
#include "board_config.h"

#include "esp_hosted_transport_config.h"  // aliases esp_hosted_* -> eh_host_*
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"
#include <string.h>

static const char *TAG = "wifi_init";

// Variáveis de estado globais (mantidas em BSS)
static bool s_wifi_initialized = false;
static bool s_wifi_connected = false;

// Event handlers
static void on_wifi_disconnect_handler(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "WiFi desconectado");
    s_wifi_connected = false;
}

static void on_wifi_connect_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "WiFi conectado ao AP");
    s_wifi_connected = true;
}

static void on_got_ip_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "IP obtido: %s", ip_str);
}

static esp_err_t configure_sdio_transport(void)
{
    /* struct esp_hosted_sdio_config é um #define pra struct
     * eh_host_sdio_config (ver esp_hosted_transport_config.h). Os
     * campos pin_* são do tipo eh_gpio_pin_t ({ port, pin }), não um
     * inteiro puro — por isso .pin_clk.pin, não .pin_clk. */
    struct esp_hosted_sdio_config transport_config = INIT_DEFAULT_HOST_SDIO_CONFIG();

    transport_config.clock_freq_khz = BOARD_C6_SDIO_FREQ_KHZ;
    transport_config.slot           = 1;  /* IMPORTANTE: o cartão SD (storage_init.c,
                                              via SDMMC_HOST_DEFAULT()) usa o slot 0.
                                              Sem isso, os dois disputam o mesmo
                                              controlador SDMMC físico — mesmo com
                                              pinos diferentes — e corrompe estado
                                              interno (viu isso na prática: assert em
                                              xQueueSemaphoreTake dentro de
                                              sdmmc_host_do_transaction, e logo depois
                                              o USB Host falhando ao registrar client,
                                              os dois provavelmente pelo mesmo motivo). */
    transport_config.pin_clk.pin    = BOARD_C6_SDIO_CLK_GPIO;
    transport_config.pin_cmd.pin    = BOARD_C6_SDIO_CMD_GPIO;
    transport_config.pin_d0.pin     = BOARD_C6_SDIO_D0_GPIO;
    transport_config.pin_d1.pin     = BOARD_C6_SDIO_D1_GPIO;
    transport_config.pin_d2.pin     = BOARD_C6_SDIO_D2_GPIO;
    transport_config.pin_d3.pin     = BOARD_C6_SDIO_D3_GPIO;
    transport_config.pin_reset.pin  = BOARD_C6_RESET_GPIO;
    /* .port fica no valor default (não usado no ESP-IDF — é uma
     * abstração multi-plataforma do componente). bus_width, rx_mode,
     * block_mode, iomux_enable e os tamanhos de fila ficam como o
     * default já veio de INIT_DEFAULT_HOST_SDIO_CONFIG(). */

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

    // Registra handlers de eventos
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                                            &on_wifi_disconnect_handler, NULL, NULL),
                        TAG, "registrar handler de disconnect");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
                                                            &on_wifi_connect_handler, NULL, NULL),
                        TAG, "registrar handler de connect");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                            &on_got_ip_handler, NULL, NULL),
                        TAG, "registrar handler de got_ip");

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "esp_wifi_set_mode");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start");

    s_wifi_initialized = true;
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

esp_err_t board_wifi_scan(wifi_ap_info_t *ap_records, uint16_t *max_count)
{
    if (!ap_records || !max_count) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t max_ap = *max_count;

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
    };

    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan_start falhou: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t num_ap = 0;
    err = esp_wifi_scan_get_ap_num(&num_ap);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan_get_ap_num falhou: %s", esp_err_to_name(err));
        return err;
    }

    if (num_ap == 0) {
        *max_count = 0;
        return ESP_OK;
    }

    // Limita ao tamanho do buffer fornecido
    uint16_t to_fetch = (num_ap > max_ap) ? max_ap : num_ap;

    wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * to_fetch);
    if (!ap_list) {
        ESP_LOGE(TAG, "sem memória pra lista de redes");
        return ESP_ERR_NO_MEM;
    }

    err = esp_wifi_scan_get_ap_records(&to_fetch, ap_list);
    if (err != ESP_OK) {
        free(ap_list);
        ESP_LOGE(TAG, "scan_get_ap_records falhou: %s", esp_err_to_name(err));
        return err;
    }

    // Copia os dados pro formato simplificado
    for (uint16_t i = 0; i < to_fetch; i++) {
        memset(ap_records[i].ssid, 0, sizeof(ap_records[i].ssid));
        memcpy(ap_records[i].ssid, ap_list[i].ssid, strnlen((const char *)ap_list[i].ssid, 32));
        ap_records[i].rssi = ap_list[i].rssi;
        ap_records[i].channel = ap_list[i].primary;
        ap_records[i].secured = (ap_list[i].authmode != WIFI_AUTH_OPEN);
        memcpy(ap_records[i].bssid, ap_list[i].bssid, 6);
    }

    *max_count = to_fetch;
    free(ap_list);
    return ESP_OK;
}

esp_err_t board_wifi_connect(const char *ssid, const char *password)
{
    if (!ssid) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi não inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);

    if (password) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }

    // Configura threshold mínimo de RSSI (-90 dBm é bem baixo, aceita quase tudo)
    wifi_config.sta.threshold.rssi = -90;

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config falhou: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_connect falhou: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Conectando a \"%s\"...", ssid);
    return ESP_OK;
}

esp_err_t board_wifi_disconnect(void)
{
    if (!s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_disconnect falhou: %s", esp_err_to_name(err));
        return err;
    }

    s_wifi_connected = false;
    ESP_LOGI(TAG, "Desconectado do WiFi");
    return ESP_OK;
}

bool board_wifi_is_connected(void)
{
    return s_wifi_connected;
}

esp_err_t board_wifi_get_ip(char *ip_string, size_t buffer_size)
{
    if (!ip_string || buffer_size < 16) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (!netif) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esp_netif_get_ip_info(netif, &ip_info);
    if (err != ESP_OK) {
        return err;
    }

    // Verifica se o IP é 0.0.0.0 (não válido)
    if (ip_info.ip.addr == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    snprintf(ip_string, buffer_size, IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}
