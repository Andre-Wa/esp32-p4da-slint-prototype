# PDA ESP32-P4 — projeto inicial (Slint + touch + teclado USB)

Esqueleto de bring-up pra placa **GUITION JC4880P433C** (ESP32-P4 + ESP32-C6),
usando **Slint (C++)** como UI e **USB Host HID** pra ler o teclado externo.

Isso **não é o PDA final** — é uma tela única que mostra a última tecla
recebida do teclado USB e um botão pra confirmar que o touch (GT911)
também está respondendo. O objetivo é validar o toolchain inteiro
(display MIPI-DSI + touch + Slint + USB Host) antes de começar a
construir os apps de verdade (notas, agenda, etc.).

## O que está confirmado vs. o que precisa verificar

Este projeto foi montado cruzando a documentação oficial do Slint pro
ESP-IDF com um BSP de código aberto feito especificamente pra essa placa
([`csvke/esp32_p4_jc4880p433c_bsp`](https://github.com/csvke/esp32_p4_jc4880p433c_bsp),
Apache 2.0). Ainda assim, **eu não tenho acesso físico à placa nem a um
compilador ESP-IDF aqui**, então nada disso foi testado de ponta a
ponta. Antes de gravar:

| Item | Status | O que fazer |
|---|---|---|
| Pinos I2C do touch (SDA=7, SCL=8) | ✅ Confirmado (fonte: BSP da comunidade) | Nenhuma ação |
| MIPI-DSI: 2 lanes, 500 Mbps, painel nativo 480×800 retrato | ✅ Confirmado | Nenhuma ação |
| GPIO de reset do painel | ✅ Confirmado: **GPIO 5** (extraído do código-fonte real do BSP de referência) | Nenhuma ação |
| GPIO de backlight | ✅ Confirmado: **GPIO 23**, via PWM/LEDC (canal 1, timer 1, 20kHz) | Nenhuma ação |
| Sequência de init vendor-specific do painel | ✅ Confirmada — copiada literalmente do BSP de referência (`s_st7701_init_cmds` em `display_init.c`) | Nenhuma ação |
| Timings de porch (h/v sync, back/front porch) e clock DPI (34MHz) | ✅ Confirmados (fonte: BSP de referência) | Nenhuma ação |
| Qual porta USB-C é a OTG (host) | ⚠️ Não confirmado | Teste as duas portas com o teclado — a errada simplesmente não vai enumerar nada, sem risco de dano |
| API exata do `esp_lcd_st7701` v1.1.5 | ⚠️ Um detalhe real já mordeu a gente: inicializador agregado (`= { .flags.x = 1, .campo = {...} }`) fez uma flag chegar como 0 em runtime nesse toolchain (GCC 14.2.0 + hardening flags do IDF). Corrigido usando atribuição sequencial em vez de inicializador agregado | Se aparecer outro campo suspeito, prefira atribuição sequencial a inicializador agregado complexo |
| Suporte do binding C++ do Slint no ESP32-P4 | ⚠️ Documentação oficial cita "testado em ESP32-S3"; P4 funciona, mas ainda é terreno novo | Espere alguma fricção de build eventualmente — é normal nessa combinação |

Ou seja: isso é um **ponto de partida sólido**, não um "clique e funciona".
Trate a primeira gravação como uma sessão de debug, não como o resultado
final.

## Estrutura

```
esp32-pda-slint/
├── CMakeLists.txt          # projeto ESP-IDF
├── partitions.csv          # tabela de partições (16MB flash)
├── sdkconfig.defaults      # PSRAM, USB-OTG, C++ exceptions/RTTI
└── main/
    ├── board_config.h      # TODOS os pinos/parâmetros de hardware, num lugar só
    ├── display_init.c/h    # bring-up do painel ST7701S via MIPI-DSI
    ├── touch_init.c/h      # bring-up do touch GT911 via I2C
    ├── usb_hid_keyboard.c++/h  # leitura do teclado via USB Host HID
    ├── main.cpp             # amarra tudo + roda o loop do Slint
    ├── idf_component.yml    # dependências (esp_lcd_st7701, gt911, usb_host_hid, slint)
    └── ui/
        └── app_ui.slint      # a tela em si (declarativa)
```

## Build

Pré-requisitos: ESP-IDF v5.3+ instalado e com `idf.py` no PATH, alvo
`esp32p4`.

```bash
cd esp32-pda-slint
idf.py set-target esp32p4
idf.py build
```

A primeira execução do `idf.py build` vai baixar as dependências do
`idf_component.yml` (inclusive o Slint) via ESP Component Manager —
precisa de internet nessa etapa.

```bash
idf.py -p /dev/ttyACM0 flash monitor   # ajuste a porta serial
```

## Se a tela não acender

1. Confira o log serial — o `ESP_LOGI`/`ESP_LOGE` em `display_init.c` vai
   dizer exatamente em qual etapa (LDO, barramento DSI, painel) travou.
2. Revise `BOARD_LCD_RST_GPIO` e `BOARD_LCD_BL_GPIO` em `board_config.h`.
3. Compare os timings de porch com os do projeto BSP de referência, se
   você conseguir abrir o `bsp_display.c` dele.

## Se o touch não responder

1. Rode um scan I2C (`i2cdetect` via um exemplo simples do IDF) pra
   confirmar que o GT911 aparece em `0x14` ou `0x5D`.
2. Se as coordenadas de toque saírem trocadas/invertidas depois que você
   rotacionar a UI pra paisagem, ajuste `swap_xy`/`mirror_x`/`mirror_y`
   em `touch_init.c`.

## Se o teclado não for detectado

1. Confirme que está usando a porta USB-C certa (a OTG, não a de
   flash/log, se forem portas diferentes).
2. O código lê o **boot protocol** de teclado — funciona com praticamente
   qualquer teclado USB padrão, incluindo composto (hub + teclado), mas
   teclados com firmware muito customizado podem precisar do protocolo
   "report" completo em vez do boot protocol.

## Próximos passos depois que isso rodar

- Trocar a tela única por um `TabWidget`/roteador simples de "telas" em
  Slint, uma por app (notas, agenda, etc.)
- Persistir dados em flash (LittleFS/SPIFFS) — o BSP de referência já
  monta SPIFFS, dá pra reaproveitar esse pedaço
- WiFi via ESP32-C6 (ESP-Hosted) pra sincronização/NTP
