# PDA ESP32-P4 (Slint + touch + storage + teclado USB)

Base funcional pra um PDA feito na placa **GUITION JC4880P433C**
(ESP32-P4 + ESP32-C6), usando **Slint (C++)** como UI, touch capacitivo,
armazenamento (flash interna + cartão SD) e teclado físico via USB.

## Escopo atual

Isso deixou de ser só um "teste de bring-up" — os pilares abaixo já
funcionam de ponta a ponta, testados em hardware real:

- **Display** MIPI-DSI (ST7701S, 480×800 nativo) renderizando em
  **paisagem** (800×480) via rotação de software do Slint, cores corretas
- **Touch** capacitivo (GT911) mapeado corretamente nos 4 cantos da tela
  já rotacionada
- **Armazenamento**: memória interna (LittleFS) e cartão microSD
  (SDMMC 4-bit) montados e navegáveis — tela "Meus Arquivos" lista os
  dois com leitura assíncrona (não trava a UI)
- **Teclado USB** físico via USB-OTG nativo (boot protocol HID),
  incluindo um fix de race condition que derrubava o firmware
- **Roteador de telas** em Slint (`AppState` + component condicional) —
  já dá pra navegar entre Launcher e outras telas

O que ainda falta pra virar o PDA de verdade: os apps em si (Notas,
Agenda, Alarmes, Contatos) por trás dos botões do launcher, que hoje só
mostram feedback de teste.

## Referências usadas

- BSP de código aberto pra essa placa específica:
  [`NickyDark1/esp32_p4_jc4880p433c_bsp`](https://github.com/NickyDark1/esp32_p4_jc4880p433c_bsp)
  (Apache 2.0) — fonte da sequência de init do painel e dos timings de
  DPI
- **Schematic oficial do fabricante** (GUITION, módulo `JC-ESP32P4-M3`) —
  fonte primária pra conferência de pinagem; onde ele diverge do BSP
  comunitário, o schematic prevalece

## Pinagem confirmada (`main/board_config.h`)

| Sinal | GPIO | Fonte |
|---|---|---|
| I2C compartilhado (touch) — SDA / SCL | 7 / 8 | BSP + schematic |
| Reset do painel LCD | 5 | BSP + schematic |
| Backlight (PWM/LEDC) | 23 | BSP, testado funcionando |
| Reset / Interrupt do touch GT911 | 22 / 20 | Schematic — **definidos mas não usados**, ver nota abaixo |
| SD: CLK / CMD / D0-D3 | 43 / 44 / 39 / 40 / 46 / 45 | Testado funcionando |
| USB-OTG (Host, teclado) | porta "High Speed USB" | Schematic — a outra porta ("Full Speed") é só USB-Serial-JTAG (flash/monitor) |

**Sobre o reset/interrupt do touch:** existem de propósito só como
`#define` em `board_config.h`, sem uso em `touch_init.c`. Testei
conectá-los — o GT911 seleciona seu próprio endereço I2C (0x14 ou 0x5D)
com base no estado do pino INT durante o pulso de reset, e sem
implementar essa sequência exata (timing do datasheet), fornecer só
`rst_gpio_num` quebra a comunicação I2C inteira (NACK). Deixados como
`GPIO_NUM_NC` — funciona via power-on-reset default do chip.

## Lições aprendidas (caras de re-aprender)

- **Ninja "manifest still dirty"**: pode ser relógio do sistema
  dessincronizado, OU arquivos com timestamp no futuro (ex: SDK
  pré-compilado do Slint extraído com timestamps estranhos — `find build
  -exec touch {} +` resolve), OU falta de `cmake_policy(SET CMP0116
  NEW)` no `main/CMakeLists.txt` (causa raiz real de parte dos casos,
  ligada ao DEPFILE do `slint_target_sources()`)
- **PSRAM do ESP32-P4 é HEX, não OCT** (`CONFIG_SPIRAM_MODE_HEX`, não
  `_OCT` — esse último nem existe pra esse chip). Sem isso, PSRAM roda a
  20MHz em vez de 200MHz e o barramento DPI do display sofre "underrun"
- Inicializador agregado (`= { .campo.sub = 1, .outro = {...} }`) pode
  fazer um campo chegar como 0 em runtime nesse toolchain (GCC 14.2.0 +
  hardening flags do IDF) — atribuição sequencial é mais segura
- Com `SlintPlatformConfiguration` + rotação ativa, `.size` é o tamanho
  **lógico pós-rotação**, não a resolução física nativa do painel — e os
  designadores (`.campo = valor`) precisam seguir a ordem exata de
  declaração da struct
- FATFS usa nomes curtos 8.3 por padrão — precisa
  `CONFIG_FATFS_LONG_FILENAMES=y` + `CONFIG_FATFS_LFN_HEAP=y`
- `clangd` no VSCode não entende flags específicas do GCC nem a extensão
  RISC-V proprietária da Espressif (`xesppie`) — precisa de um `.clangd`
  filtrando essas flags, **e** `--query-driver` nas configs do VSCode
  apontando pro toolchain real, senão ele não acha nem `<vector>`

## Estrutura

```
esp32-pda-slint/
├── CMakeLists.txt          # projeto ESP-IDF
├── partitions.csv          # tabela de partições (16MB flash, com partição pra LittleFS)
├── sdkconfig.defaults      # PSRAM (HEX/200MHz), USB-OTG, FATFS LFN, cache/perf pro DPI
└── main/
    ├── board_config.h      # TODOS os pinos/parâmetros de hardware, num lugar só
    ├── display_init.c/h    # bring-up do painel ST7701S via MIPI-DSI
    ├── touch_init.c/h      # bring-up do touch GT911 via I2C
    ├── storage_init.c/h    # LittleFS interno + SDMMC (cartão SD)
    ├── usb_hid_keyboard.cpp/h  # leitura do teclado via USB Host HID
    ├── main.cpp             # amarra tudo + roda o loop do Slint
    ├── idf_component.yml    # dependências (esp_lcd_st7701, gt911, usb_host_hid, slint)
    └── ui/
        └── app_ui.slint      # roteador de telas + launcher + gerenciador de arquivos
```

## Build

Pré-requisitos: ESP-IDF v5.5.1 instalado e com `idf.py` no PATH, alvo
`esp32p4`. Veja [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/v6.0.2/esp32p4/get-started/index.html) para mais detalhes.

```bash
cd esp32-pda-slint
idf.py set-target esp32p4
idf.py build
idf.py -p /dev/ttyACM0 flash monitor   # ajuste a porta serial
```

A primeira execução do `idf.py build` vai baixar as dependências do
`idf_component.yml` (inclusive o Slint) via ESP Component Manager —
precisa de internet nessa etapa.

Para eventuais correções, apagar os arquivos não essenciais pode ser uma correção rápida:

```bash
cd esp32-pda-slint
rm -rf build managed_components dependencies.lock sdkconfig sdkconfig.old
# Execute os comandos de build novamente
```

### IDE (VSCode + clangd)

Se usar a extensão `clangd`, crie `.clangd` na raiz do projeto:

```yaml
CompileFlags:
  CompilationDatabase: build
  Remove:
    - -fstrict-volatile-bitfields
    - -fno-tree-switch-conversion
    - -march=*
  Add:
    - -march=rv32imafc_zicsr_zifencei
```

E em `.vscode/settings.json`, aponte o `--query-driver` pro toolchain
real (ajuste o caminho pro seu usuário/instalação):

```json
{
    "idf.currentSetup": "~/.espressif/VERSION/esp-idf",
    
    "clangd.arguments": [
        "--query-driver=~/.espressif/tools/riscv32-esp-elf/**"
    ]
}
```

Alternativa mais simples: usar a extensão oficial **ESP-IDF** da
Espressif em vez de `clangd` puro — lida melhor com isso por padrão.

## Troubleshooting

**Tela não acende:** confira o log serial (`ESP_LOGE` em
`display_init.c` aponta a etapa exata — LDO, barramento DSI, painel).
Revise `BOARD_LCD_RST_GPIO`/`BOARD_LCD_BL_GPIO`.

**Touch não responde ou coordenadas erradas:** confirme o GT911 em
`0x14`/`0x5D` via scan I2C. Se mudar a rotação da UI, revise
`swap_xy`/`mirror_x`/`mirror_y` em `touch_init.c` (testando os 4 cantos
da tela, não só um botão central).

**SD não monta:** confira o log (`storage_init.c` loga cada etapa —
energização do LDO, montagem SDMMC). Cartões muito antigos/lentos podem
precisar de `SDMMC_FREQ_DEFAULT` em vez de `HIGHSPEED`.

**Teclado não é detectado ou derruba o firmware ao digitar:** confirme
a porta USB-C certa ("High Speed", não a de flash/monitor). Se
travar especificamente ao digitar logo após conectar, confira se o
filtro de interface HID em `usb_hid_keyboard.cpp`
(`dev_params.proto != HID_PROTOCOL_KEYBOARD`) está presente — sem ele,
adaptadores OTG que expõem uma segunda interface HID vestigial podem
causar uma race condition real (já vimos isso acontecer).

**Erros vermelhos no VSCode mas o firmware compila normal:** é o
`clangd` confuso, não o build de verdade. Ver seção de IDE acima.

## Próximos passos

- Construir as telas reais por trás dos botões do launcher: Notas
  (usando o storage já pronto), Agenda, Alarmes, Contatos
- WiFi via ESP32-C6 (ESP-Hosted) pra sincronização/NTP — ainda não
  investigado nessa sessão
- Investigar se o par I2C `RTC_DAT/SDA1`/`RTC_CLK/SCL1` (GPIO29/30) no
  schematic é mesmo um RTC de hardware dedicado — resolveria manter hora
  certa sem depender de WiFi
- Áudio (ES8311 + ES7210 + amplificador) pra som de alarme/notificação —
  hardware presente no schematic, não usado ainda
- Leitura de nível de bateria via o chip de gerenciamento de energia
  (IP5306) visível no schematic