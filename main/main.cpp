/**
 * main.cpp
 *
 * Amarra tudo: painel MIPI-DSI (cru) + touch GT911 (cru) entram no
 * slint_esp_init(); daí em diante quem desenha é o Slint. 
 */

#include "slint-esp.h"
#include "app_ui.h"   // gerado automaticamente a partir de ui/app_ui.slint

#include "board_config.h"
#include "display_init.h"
#include "touch_init.h"
#include "usb_hid_keyboard.h"
#include "storage_init.h"
#include "wifi_init.h"

#include "esp_log.h"
#include <private/slint_size.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <dirent.h>
#include <thread>     // <--- ADICIONADO: Para I/O Assíncrono

static const char *TAG = "main";

/** Gera "notaN" com N = maior número já usado + 1, escaneando
 *  /internal/notes. Sem RTC/hora confiável ainda, então usar um nome
 *  numerado sequencial em vez de timestamp. */
static std::string generate_new_note_name(void)
{
    int max_n = 0;
    DIR *dir = opendir("/internal/notes");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            std::string name(ent->d_name);
            if (name.rfind("nota", 0) == 0) {
                size_t dot = name.find(".txt");
                if (dot != std::string::npos && dot > 4) {
                    int n = atoi(name.substr(4, dot - 4).c_str());
                    if (n > max_n) {
                        max_n = n;
                    }
                }
            }
        }
        closedir(dir);
    }
    return "nota" + std::to_string(max_n + 1);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "iniciando bring-up...");

    // Inicializa Memórias
    board_storage_init();

    // Inicializa Display
    esp_lcd_panel_handle_t panel = nullptr;
    ESP_ERROR_CHECK(board_display_init(&panel));
    board_display_backlight_on();

    // Inicializa Touch
    esp_lcd_touch_handle_t touch = nullptr;
    ESP_ERROR_CHECK(board_touch_init(&touch));

    // Inicializa WiFi (via ESP32-C6/SDIO) — experimental, não trava o
    // boot se falhar. Veja o log: se aparecer "Version mismatch", é o
    // firmware desatualizado do C6 (ver README, seção WiFi/C6).
    esp_err_t wifi_err = board_wifi_init();
    if (wifi_err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi não inicializou (%s) — continuando sem WiFi por enquanto", esp_err_to_name(wifi_err));
    }

    // Framebuffer
    static std::vector<slint::platform::Rgb565Pixel> framebuffer(
        BOARD_LCD_H_RES_NATIVE * BOARD_LCD_V_RES_NATIVE);

    // Inicializa o Motor UI (Slint)
    slint_esp_init(SlintPlatformConfiguration<slint::platform::Rgb565Pixel>{
        .size = slint::PhysicalSize({BOARD_LCD_V_RES_NATIVE, BOARD_LCD_H_RES_NATIVE}),
        .panel_handle = panel,
        .touch_handle = touch,
        .buffer1 = framebuffer,
        .rotation = slint::platform::SoftwareRenderer::RenderingRotation::Rotate90,
        .byte_swap = false,
    });

    auto ui = AppWindow::create();

    // --- LEITURA DE ARQUIVOS (ASSÍNCRONA) ---
    ui->on_request_file_list([ui]() {
        // Dispara uma thread em background (não congela a animação dos botões da interface)
        std::thread([ui]() {
            // Cria o modelo dinâmico na thread
            auto file_model = std::make_shared<slint::VectorModel<slint::SharedString>>();
            
            // Função lambda interna para ler o disco
            auto read_dir = [&](const char* path, const char* label) {
                file_model->push_back(slint::SharedString(label)); 
                
                DIR *dir = opendir(path);
                if (dir != NULL) {
                    struct dirent *ent;
                    int count = 0;
                    while ((ent = readdir(dir)) != NULL) {
                        std::string name = std::string("  ") + ent->d_name;
                        if (ent->d_type == DT_DIR) name += "/"; 
                        file_model->push_back(slint::SharedString(name));
                        count++;
                    }
                    if (count == 0) file_model->push_back(slint::SharedString("  (Vazio)"));
                    closedir(dir);
                } else {
                    file_model->push_back(slint::SharedString("  (Não Montado)"));
                }
            };

            // Lendo o disco físico (Pode demorar vários milissegundos)
            read_dir("/internal", "💾 Memória Interna:");
            file_model->push_back(slint::SharedString("")); // Espaço
            read_dir("/sdcard", "💽 Cartão SD:");

            // Devolve o resultado de forma segura para a Thread Principal da UI
            slint::invoke_from_event_loop([ui, file_model]() {
                ui->set_file_list(file_model);
            });
            
        }).detach(); // .detach() permite que a thread rode livremente e morra sozinha
    });
    // ----------------------------------------

    // --- NOTAS ---
    ui->on_request_notes_list([ui]() {
        std::thread([ui]() {
            auto notes_model = std::make_shared<slint::VectorModel<slint::SharedString>>();

            DIR *dir = opendir("/internal/notes");
            int count = 0;
            if (dir != NULL) {
                struct dirent *ent;
                while ((ent = readdir(dir)) != NULL) {
                    std::string name(ent->d_name);
                    if (name == "." || name == "..") continue;
                    // remove a extensão ".txt" pra exibir só o nome
                    if (name.size() > 4 && name.compare(name.size() - 4, 4, ".txt") == 0) {
                        name = name.substr(0, name.size() - 4);
                    }
                    notes_model->push_back(slint::SharedString(name));
                    count++;
                }
                closedir(dir);
            }
            if (count == 0) {
                notes_model->push_back(slint::SharedString("(nenhuma nota ainda — toque em + Nova nota)"));
            }

            slint::invoke_from_event_loop([ui, notes_model]() {
                ui->set_notes_list(notes_model);
            });
        }).detach();
    });

    ui->on_open_note([ui](slint::SharedString name) {
        std::string filename = "/internal/notes/" + std::string(name.data()) + ".txt";
        std::vector<char> buf(8192);
        size_t len = 0;
        storage_read_text_file(filename.c_str(), buf.data(), buf.size(), &len);

        ui->set_note_name(name);
        ui->set_note_buffer(slint::SharedString(std::string(buf.data(), len)));
        ui->set_note_is_new(false);
        ui->set_active_app(AppState::NoteEdit);
    });

    ui->on_new_note([ui]() {
        std::string name = generate_new_note_name();
        ui->set_note_name(slint::SharedString(name));
        ui->set_note_buffer(slint::SharedString(""));
        ui->set_note_is_new(true);
        ui->set_active_app(AppState::NoteEdit);
    });

    ui->on_save_note([ui]() {
        std::string name(ui->get_note_name().data());
        std::string content(ui->get_note_buffer().data());
        std::string filename = "/internal/notes/" + name + ".txt";

        esp_err_t err = storage_write_text_file(filename.c_str(), content.data(), content.size());
        if (err == ESP_OK) {
            ui->set_note_is_new(false);
            ESP_LOGI(TAG, "nota salva: %s (%u bytes)", filename.c_str(), (unsigned)content.size());
        } else {
            ESP_LOGE(TAG, "falha ao salvar nota: %s", filename.c_str());
        }
    });

    ui->on_delete_note([ui]() {
        std::string name(ui->get_note_name().data());
        std::string filename = "/internal/notes/" + name + ".txt";
        storage_delete_file(filename.c_str());
        ui->set_active_app(AppState::NotesList);
        ui->invoke_request_notes_list();
    });
    // ----------------------------------------

    // --- TECLADO USB ---
    usb_hid_keyboard_init([ui](uint8_t ascii, uint8_t keycode, uint8_t /*modifiers*/) {
        char buf[16];
        if (ascii == '\n')      snprintf(buf, sizeof(buf), "Enter");
        else if (ascii == '\b') snprintf(buf, sizeof(buf), "Backspace");
        else if (ascii == '\t') snprintf(buf, sizeof(buf), "Tab");
        else if (ascii == ' ')  snprintf(buf, sizeof(buf), "Espaco");
        else if (ascii != 0)    snprintf(buf, sizeof(buf), "%c", ascii);
        else                    snprintf(buf, sizeof(buf), "0x%02X", keycode);

        std::string label(buf);
        slint::invoke_from_event_loop([ui, label, ascii]() {
            ui->set_last_key(slint::SharedString(label));

            // Só edita o texto da nota se a tela de edição estiver
            // ativa — em qualquer outra tela, a tecla só atualiza o
            // "last_key" acima (útil pra debug), sem outro efeito.
            if (ui->get_active_app() == AppState::NoteEdit && ascii != 0) {
                std::string text(ui->get_note_buffer().data());
                if (ascii == '\b') {
                    if (!text.empty()) {
                        text.pop_back();
                    }
                } else if (ascii == '\t') {
                    // Tab dentro de uma nota: sem efeito por enquanto.
                } else {
                    text += (char)ascii;
                }
                ui->set_note_buffer(slint::SharedString(text));
            }
        });
    });

    ESP_LOGI(TAG, "bring-up completo, entrando no loop do Slint");
    ui->run();
}