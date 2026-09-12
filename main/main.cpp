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
#include "clock_init.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <private/slint_size.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <dirent.h>
#include <thread>     // <--- ADICIONADO: Para I/O Assíncrono

static const char *TAG = "main";

// Estado WiFi mantido no C++
static std::string s_wifi_ssid;
static std::string s_wifi_password;
static bool s_wifi_scanning = false;

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

    // --- RELÓGIO E BRILHO ---
    static int s_brightness = 100;

    ui->on_request_brightness_up([ui]() {
        s_brightness = (s_brightness + 10 > 100) ? 100 : s_brightness + 10;
        board_display_set_brightness((uint8_t)s_brightness);
        ui->set_clock_brightness(s_brightness);
    });

    ui->on_request_brightness_down([ui]() {
        // Não deixa ir abaixo de 10% pra não apagar a tela sem querer
        // (sem outro jeito de "ver" e corrigir isso de volta).
        s_brightness = (s_brightness - 10 < 10) ? 10 : s_brightness - 10;
        board_display_set_brightness((uint8_t)s_brightness);
        ui->set_clock_brightness(s_brightness);
    });

    ui->on_confirm_set_time([ui]() {
        std::string buf(ui->get_clock_set_time_buffer().data());
        if (buf.size() != 10) {
            ESP_LOGW(TAG, "formato de data/hora inválido: \"%s\" (esperado 10 dígitos DDMMAAHHMM)", buf.c_str());
            return;
        }
        int day    = atoi(buf.substr(0, 2).c_str());
        int month  = atoi(buf.substr(2, 2).c_str());
        int year   = 2000 + atoi(buf.substr(4, 2).c_str());
        int hour   = atoi(buf.substr(6, 2).c_str());
        int minute = atoi(buf.substr(8, 2).c_str());
        board_clock_set(year, month, day, hour, minute, 0);
        ESP_LOGI(TAG, "hora ajustada manualmente: %02d/%02d/%04d %02d:%02d", day, month, year, hour, minute);
    });

    // Task que atualiza o mostrador do relógio a cada segundo — roda
    // sempre, mesmo fora da tela de Relógio (custo desprezível: só
    // formata uma string e atualiza uma propriedade).
    std::thread([ui]() {
        while (true) {
            char buf[32];
            board_clock_get_string(buf, sizeof(buf));
            std::string s(buf);
            slint::invoke_from_event_loop([ui, s]() {
                ui->set_clock_time_display(slint::SharedString(s));
            });
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }).detach();
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

            // Mesma lógica pra digitar a senha do WiFi. A máscara
            // (pontos) é calculada aqui, não no Slint — o Slint não tem
            // como fazer um loop imperativo dentro de uma expressão,
            // só sabe repetir elemento de UI com "for".
            if (ui->get_active_app() == AppState::WifiPassword && ascii != 0) {
                std::string pass(ui->get_wifi_password_buffer().data());
                if (ascii == '\b') {
                    if (!pass.empty()) {
                        pass.pop_back();
                    }
                } else if (ascii == '\n' || ascii == '\t') {
                    // Enter/Tab não inserem caractere na senha.
                } else {
                    pass += (char)ascii;
                }
                ui->set_wifi_password_buffer(slint::SharedString(pass));
                ui->set_wifi_password_mask(slint::SharedString(std::string(pass.size(), '*')));
            }

            // Ajuste manual de data/hora: só aceita dígitos (0-9),
            // já que o formato esperado é DDMMAAHHMM.
            if (ui->get_active_app() == AppState::ClockSetTime) {
                std::string buf(ui->get_clock_set_time_buffer().data());
                if (ascii == '\b') {
                    if (!buf.empty()) {
                        buf.pop_back();
                    }
                } else if (ascii >= '0' && ascii <= '9' && buf.size() < 10) {
                    buf += (char)ascii;
                }
                ui->set_clock_set_time_buffer(slint::SharedString(buf));
            }
        });
    });
    // ----------------------------------------

    // --- WiFi ---
    ui->on_scan_wifi([ui]() {
        std::thread([ui]() {
            s_wifi_scanning = true;
            slint::invoke_from_event_loop([ui]() {
                ui->set_wifi_scanning(true);
                ui->set_wifi_status("Escaneando redes...");
            });

            // Scan usando a API C
            wifi_ap_info_t networks[20];
            uint16_t max_count = 20;
            esp_err_t err = board_wifi_scan(networks, &max_count);

            auto wifi_model = std::make_shared<slint::VectorModel<WifiNetwork>>();
            if (err == ESP_OK) {
                for (uint16_t i = 0; i < max_count; i++) {
                    wifi_model->push_back(WifiNetwork{
                        .ssid = slint::SharedString(networks[i].ssid),
                        .rssi = networks[i].rssi,
                        .secured = networks[i].secured
                    });
                }
                slint::invoke_from_event_loop([ui, wifi_model, max_count]() {
                    ui->set_wifi_networks(wifi_model);
                    ui->set_wifi_scanning(false);
                    ui->set_wifi_status(max_count > 0
                        ? slint::SharedString(std::to_string(max_count) + " rede(s) encontrada(s)")
                        : slint::SharedString("Nenhuma rede encontrada"));
                });
            } else {
                slint::invoke_from_event_loop([ui]() {
                    ui->set_wifi_scanning(false);
                    ui->set_wifi_status(slint::SharedString("Erro ao escanear"));
                });
            }
        }).detach();
    });

    ui->on_connect_wifi([ui](slint::SharedString ssid, slint::SharedString password) {
        std::string ssid_str(ssid.data());
        std::string password_str(password.data());

        s_wifi_ssid = ssid_str;
        s_wifi_password = password_str;

        slint::invoke_from_event_loop([ui, ssid]() {
            ui->set_wifi_status(slint::SharedString("Conectando a " + std::string(ssid.data()) + "..."));
        });

        esp_err_t err = board_wifi_connect(ssid_str.c_str(), password_str.empty() ? nullptr : password_str.c_str());

        slint::invoke_from_event_loop([ui, err, ssid_str]() {
            if (err == ESP_OK) {
                ui->set_wifi_status(slint::SharedString("Conectado a " + ssid_str));
                ui->set_wifi_connected(true);
                ui->set_wifi_ssid(slint::SharedString(ssid_str));
            } else {
                ui->set_wifi_status(slint::SharedString("Falha ao conectar"));
                ui->set_wifi_connected(false);
            }
        });
    });

    ui->on_disconnect_wifi([ui]() {
        esp_err_t err = board_wifi_disconnect();

        slint::invoke_from_event_loop([ui, err]() {
            if (err == ESP_OK) {
                ui->set_wifi_status(slint::SharedString("Desconectado"));
                ui->set_wifi_connected(false);
                ui->set_wifi_ssid(slint::SharedString(""));
            } else {
                ui->set_wifi_status(slint::SharedString("Erro ao desconectar"));
            }
        });
    });
    // ----------------------------------------

    ESP_LOGI(TAG, "bring-up completo, entrando no loop do Slint");
    ui->run();
}