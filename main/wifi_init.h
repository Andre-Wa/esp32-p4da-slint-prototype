#pragma once
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sobe o transporte SDIO com o coprocessador ESP32-C6 e inicializa o
 * stack WiFi por cima dele (via esp_hosted + esp_wifi_remote).
 *
 * TESTE MÍNIMO por enquanto: só sobe o transporte e faz um scan de
 * redes — não conecta em nada ainda. Serve pra confirmar se o link
 * SDIO com o C6 está saudável antes de escrever a tela de configuração
 * de verdade.
 */
esp_err_t board_wifi_init(void);

/**
 * Realiza um scan de redes WiFi disponíveis e loga os resultados.
 * Usado pra teste inicial de bring-up.
 */
void board_wifi_scan_and_log(void);

/**
 * Estrutura pra guardar informações de uma rede WiFi encontrada
 */
typedef struct {
    char ssid[33];      // SSID (até 32 caracteres + null terminator)
    int8_t rssi;        // Força do sinal (-100 a 0 dBm)
    uint8_t channel;    // Canal (1-14)
    uint8_t bssid[6];   // MAC address do AP
    bool secured;       // false = rede aberta (sem senha), true = protegida
} wifi_ap_info_t;

/**
 * Realiza um scan de redes WiFi e retorna a lista de APs encontrados.
 *
 * @param[out] ap_records  Ponteiro para array de wifi_ap_info_t que será preenchido
 * @param[inout] max_count  Na entrada: tamanho máximo do array ap_records
 *                          Na saída: número real de redes encontradas
 * @return ESP_OK se sucesso, erro caso contrário
 */
esp_err_t board_wifi_scan(wifi_ap_info_t *ap_records, uint16_t *max_count);

/**
 * Conecta a uma rede WiFi usando as credenciais fornecidas.
 *
 * @param ssid Nome da rede (SSID)
 * @param password Senha da rede (pode ser NULL para redes abertas)
 * @return ESP_OK se sucesso, erro caso contrário
 */
esp_err_t board_wifi_connect(const char *ssid, const char *password);

/**
 * Desconecta da rede WiFi atual.
 * @return ESP_OK se sucesso, erro caso contrário
 */
esp_err_t board_wifi_disconnect(void);

/**
 * Verifica se está conectado a uma rede WiFi.
 *
 * @return true se conectado, false caso contrário
 */
bool board_wifi_is_connected(void);

/**
 * Obtém o endereço IP atual (se conectado).
 *
 * @param[out] ip_string Buffer pra guardar o IP em formato string (ex: "192.168.1.100")
 * @param[in] buffer_size Tamanho do buffer ip_string
 * @return ESP_OK se tem IP válido, erro caso contrário
 */
esp_err_t board_wifi_get_ip(char *ip_string, size_t buffer_size);

#ifdef __cplusplus
}
#endif