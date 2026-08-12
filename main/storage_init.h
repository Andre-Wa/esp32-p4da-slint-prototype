#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Monta o LittleFS na partição "storage" no caminho "/internal"
 * Monta o Cartão MicroSD (se inserido) no caminho "/sdcard"
 */
esp_err_t board_storage_init(void);

#ifdef __cplusplus
}
#endif