/**
 * main.cpp
 *
 * Amarra tudo: painel MIPI-DSI (cru) + touch GT911 (cru) entram no
 * slint_esp_init(); daí em diante quem desenha é o Slint. O teclado USB
 * roda numa task separada e empurra teclas pra UI via
 * slint::invoke_from_event_loop (IMPORTANTE: nunca mexa direto numa
 * propriedade do Slint a partir de outra task — o motor de UI não é
 * thread-safe por padrão, só o invoke_from_event_loop garante isso).
 */

#include "slint-esp.h"
#include "app_ui.h"   // gerado automaticamente a partir de ui/app_ui.slint

#include "board_config.h"
#include "display_init.h"
#include "touch_init.h"
#include "usb_hid_keyboard.h"

#include "esp_log.h"
#include <private/slint_size.h>
#include <vector>
#include <cstdio>
#include <string>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "iniciando bring-up: display -> touch -> slint -> usb");

    esp_lcd_panel_handle_t panel = nullptr;
    ESP_ERROR_CHECK(board_display_init(&panel));
    board_display_backlight_on();

    esp_lcd_touch_handle_t touch = nullptr;
    ESP_ERROR_CHECK(board_touch_init(&touch));

    // Framebuffer RGB565 do tamanho nativo do painel (retrato). Fica em
    // PSRAM automaticamente por ser static + grande o suficiente pro
    // alocador preferir SPIRAM.
    static std::vector<slint::platform::Rgb565Pixel> framebuffer(
        BOARD_LCD_H_RES_NATIVE * BOARD_LCD_V_RES_NATIVE);

    slint_esp_init(SlintPlatformConfiguration<slint::platform::Rgb565Pixel>{
        .size = slint::PhysicalSize({BOARD_LCD_V_RES_NATIVE, BOARD_LCD_H_RES_NATIVE}),
        .panel_handle = panel,
        .touch_handle = touch,
        .buffer1 = framebuffer,
        .rotation = slint::platform::SoftwareRenderer::RenderingRotation::Rotate90,
        .byte_swap = false,
    });

    auto ui = AppWindow::create();

    usb_hid_keyboard_init([ui](uint8_t ascii, uint8_t keycode, uint8_t /*modifiers*/) {
        char buf[16];
        if (ascii == '\n')      std::snprintf(buf, sizeof(buf), "Enter");
        else if (ascii == '\b') std::snprintf(buf, sizeof(buf), "Backspace");
        else if (ascii == '\t') std::snprintf(buf, sizeof(buf), "Tab");
        else if (ascii == ' ')  std::snprintf(buf, sizeof(buf), "Espaco");
        else if (ascii != 0)    std::snprintf(buf, sizeof(buf), "%c", ascii);
        else                    std::snprintf(buf, sizeof(buf), "0x%02X", keycode);

        std::string label(buf);
        // Marshal de volta pra thread da UI do Slint — obrigatório.
        slint::invoke_from_event_loop([ui, label]() {
            ui->set_last_key(slint::SharedString(label));
        });
    });

    ESP_LOGI(TAG, "bring-up completo, entrando no loop do Slint");
    ui->run();
}
