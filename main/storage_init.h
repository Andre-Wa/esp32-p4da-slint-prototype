#pragma once
#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Monta o LittleFS na partição "storage" no caminho "/internal"
 * Monta o Cartão MicroSD (se inserido) no caminho "/sdcard"
 * Também garante que "/internal/notes" existe.
 */
esp_err_t board_storage_init(void);

/**
 * Helpers genéricos de arquivo — funcionam em qualquer caminho montado
 * (/internal/... ou /sdcard/...), já que o VFS do ESP-IDF trata os dois
 * com a mesma API POSIX (fopen/fread/fwrite).
 */

/** Lê um arquivo de texto inteiro pra out_buf (até buf_size-1 bytes,
 *  sempre termina com '\0'). *out_len recebe o tamanho real lido (sem
 *  contar o '\0'). Se o arquivo for maior que buf_size, trunca (loga um
 *  aviso). */
esp_err_t storage_read_text_file(const char *path, char *out_buf, size_t buf_size, size_t *out_len);

/** Escreve (sobrescrevendo) um arquivo de texto com exatamente `len`
 *  bytes de `data`. */
esp_err_t storage_write_text_file(const char *path, const char *data, size_t len);

/** Apaga um arquivo. Não é erro fatal se o arquivo não existir. */
esp_err_t storage_delete_file(const char *path);

/** Garante que um diretório existe (mkdir idempotente — ignora erro se
 *  já existir). */
esp_err_t storage_ensure_dir(const char *path);

#ifdef __cplusplus
}
#endif