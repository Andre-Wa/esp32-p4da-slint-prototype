/**
 * touch_init.c
 *
 * Inicializa o I2C compartilhado e o controlador de touch GT911.
 * Pinos I2C (SDA=7, SCL=8) confirmados pelo BSP de referência da
 * comunidade (ver board_config.h).
 */

#include "touch_init.h"
#include "board_config.h"

#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_panel_io.h"

static const char *TAG = "touch_init";

esp_err_t board_touch_init(esp_lcd_touch_handle_t *out_touch)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA_GPIO,
        .scl_io_num = BOARD_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false, /* placa já tem pull-up externo */
    };
    i2c_master_bus_handle_t bus_handle = NULL;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &bus_handle), TAG, "criar barramento I2C");

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = BOARD_I2C_CLK_HZ;

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(bus_handle, &tp_io_config, &tp_io_handle),
                         TAG, "criar panel_io I2C do GT911");

    /* x_max/y_max na orientação NATIVA (retrato) do painel. Se você
     * rotacionar a UI para paisagem no Slint, ajuste swap_xy/mirror_*
     * aqui pra bater com a rotação — senão os toques ficam em coordenada
     * errada. */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = BOARD_LCD_H_RES_NATIVE,
        .y_max = BOARD_LCD_V_RES_NATIVE,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };

    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, out_touch),
                         TAG, "criar driver GT911");

    ESP_LOGI(TAG, "GT911 inicializado no I2C%d (SDA=%d SCL=%d)",
             BOARD_I2C_PORT, BOARD_I2C_SDA_GPIO, BOARD_I2C_SCL_GPIO);
    return ESP_OK;
}
