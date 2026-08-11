#pragma once

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Inicializa o barramento MIPI-DSI + painel ST7701S.
 *
 * Devolve o handle "cru" do painel (esp_lcd_panel_handle_t), sem passar
 * por nenhum framework de UI — é isso que o slint_esp_init() espera
 * receber.
 */
esp_err_t board_display_init(esp_lcd_panel_handle_t *out_panel);

/** Liga o backlight (se BOARD_LCD_BL_GPIO estiver definido). */
void board_display_backlight_on(void);

#ifdef __cplusplus
}
#endif
