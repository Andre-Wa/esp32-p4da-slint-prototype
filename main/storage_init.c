#include "storage_init.h"
#include "board_config.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_ldo_regulator.h"
#include "esp_littlefs.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static const char *TAG = "storage_init";
static esp_ldo_channel_handle_t s_ldo_sd = NULL;

static esp_err_t init_internal_littlefs(void)
{
    ESP_LOGI(TAG, "Montando LittleFS interno em /internal...");
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/internal",
        .partition_label = "storage", // Nome da partição no partitions.csv
        .format_if_mount_failed = true, // Formata na primeira vez
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar LittleFS (%s)", esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t init_sdcard_sdmmc(void)
{
    ESP_LOGI(TAG, "Ligando energia do MicroSD (LDO4)...");
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = BOARD_SD_LDO_CHAN,
        .voltage_mv = BOARD_SD_LDO_MV,
    };
    esp_err_t ret = esp_ldo_acquire_channel(&ldo_cfg, &s_ldo_sd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ligar LDO do SD: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Montando MicroSD em /sdcard via SDMMC (4-bit)...");
    
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    // Padrão é 20MHz. Se o cartão for bom, podemos subir para SDMMC_FREQ_HIGHSPEED depois
    host.max_freq_khz = SDMMC_FREQ_PROBING; 

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = BOARD_SD_CLK_GPIO;
    slot_config.cmd = BOARD_SD_CMD_GPIO;
    slot_config.d0 = BOARD_SD_D0_GPIO;
    slot_config.d1 = BOARD_SD_D1_GPIO;
    slot_config.d2 = BOARD_SD_D2_GPIO;
    slot_config.d3 = BOARD_SD_D3_GPIO;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, // Não queremos formatar o cartão do usuário por acidente
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "MicroSD nao montado (Cartão ausente ou erro: %s)", esp_err_to_name(ret));
        return ret; // Não é erro fatal, o usuário pode ligar o PDA sem cartão
    }

    ESP_LOGI(TAG, "MicroSD montado com sucesso!");
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}

esp_err_t board_storage_init(void)
{
    init_internal_littlefs();
    init_sdcard_sdmmc();
    storage_ensure_dir("/internal/notes");
    return ESP_OK; // Sempre retorna OK para não travar o boot se não houver cartão
}

esp_err_t storage_ensure_dir(const char *path)
{
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        ESP_LOGE(TAG, "mkdir(%s) falhou: %s", path, strerror(errno));
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t storage_read_text_file(const char *path, char *out_buf, size_t buf_size, size_t *out_len)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGW(TAG, "storage_read_text_file: não abriu %s (%s)", path, strerror(errno));
        return ESP_FAIL;
    }
    size_t n = fread(out_buf, 1, buf_size - 1, f);
    fclose(f);
    out_buf[n] = '\0';
    if (out_len) {
        *out_len = n;
    }
    return ESP_OK;
}

esp_err_t storage_write_text_file(const char *path, const char *data, size_t len)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "storage_write_text_file: não abriu %s (%s)", path, strerror(errno));
        return ESP_FAIL;
    }
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    if (written != len) {
        ESP_LOGE(TAG, "storage_write_text_file: escreveu %u/%u bytes em %s", (unsigned)written, (unsigned)len, path);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t storage_delete_file(const char *path)
{
    if (remove(path) != 0 && errno != ENOENT) {
        ESP_LOGW(TAG, "storage_delete_file: falha ao apagar %s (%s)", path, strerror(errno));
        return ESP_FAIL;
    }
    return ESP_OK;
}