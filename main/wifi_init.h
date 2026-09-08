#pragma once
#include "esp_err.h"

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

#ifdef __cplusplus
}
#endif