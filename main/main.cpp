#include "slint-esp.h"
#include "app_ui.h"   // gerado automaticamente a partir de ui/app_ui.slint

#include "board_config.h"
#include "display_init.h"
#include "touch_init.h"
#include "usb_hid_keyboard.h"
#include "storage_init.h"

#include "esp_log.h"
#include <private/slint_size.h>
#include <vector>
#include <cstdio>
#include <string>
#include <dirent.h>  // <--- ADICIONAR ESTA LINHA PARA LER DIRETÓRIOS

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "iniciando bring-up...");

    board_storage_init();

    esp_lcd_panel_handle_t panel = nullptr;
    ESP_ERROR_CHECK(board_display_init(&panel));
    board_display_backlight_on();

    esp_lcd_touch_handle_t touch = nullptr;
    ESP_ERROR_CHECK(board_touch_init(&touch));

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

    // --- NOVA LÓGICA: Leitura de Arquivos ao abrir o File Manager ---
    ui->on_request_file_list([ui]() {
        // Cria um novo modelo de vetor que será enviado ao Slint
        auto file_model = std::make_shared<slint::VectorModel<slint::SharedString>>();
        
        // Função lambda auxiliar para ler um diretório específico
        auto read_dir = [&](const char* path, const char* label) {
            file_model->push_back(slint::SharedString(label)); // Título da seção
            
            DIR *dir = opendir(path);
            if (dir != NULL) {
                struct dirent *ent;
                int count = 0;
                while ((ent = readdir(dir)) != NULL) {
                    std::string name = std::string("  ") + ent->d_name;
                    if (ent->d_type == DT_DIR) name += "/"; // Coloca uma barra em pastas
                    file_model->push_back(slint::SharedString(name));
                    count++;
                }
                if (count == 0) file_model->push_back(slint::SharedString("  (Vazio)"));
                closedir(dir);
            } else {
                file_model->push_back(slint::SharedString("  (Não Montado)"));
            }
        };

        // Lê a memória interna (LittleFS)
        read_dir("/internal", "💾 Memória Interna:");
        file_model->push_back(slint::SharedString("")); // linha em branco
        
        // Lê o MicroSD
        read_dir("/sdcard", "💽 Cartão SD:");

        // Envia os dados estruturados para o Slint (deve ser feito no Event Loop)
        slint::invoke_from_event_loop([ui, file_model]() {
            ui->set_file_list(file_model);
        });
    });
    // ----------------------------------------------------------------

    usb_hid_keyboard_init([ui](uint8_t ascii, uint8_t keycode, uint8_t /*modifiers*/) {
        char buf[16];
        if (ascii == '\n')      std::snprintf(buf, sizeof(buf), "Enter");
        else if (ascii == '\b') std::snprintf(buf, sizeof(buf), "Backspace");
        else if (ascii == '\t') std::snprintf(buf, sizeof(buf), "Tab");
        else if (ascii == ' ')  std::snprintf(buf, sizeof(buf), "Espaco");
        else if (ascii != 0)    std::snprintf(buf, sizeof(buf), "%c", ascii);
        else                    std::snprintf(buf, sizeof(buf), "0x%02X", keycode);

        std::string label(buf);
        slint::invoke_from_event_loop([ui, label]() {
            ui->set_last_key(slint::SharedString(label));
        });
    });

    ESP_LOGI(TAG, "bring-up completo, entrando no loop do Slint");
    ui->run();
}
