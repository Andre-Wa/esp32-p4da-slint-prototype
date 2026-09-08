/**
 * board_config.h
 *
 * Parâmetros de hardware da GUITION JC4880P433C (ESP32-P4 + ESP32-C6).
 *
 * Valores confirmados via leitura direta do código-fonte do BSP de
 * referência para essa placa:
 *   https://github.com/NickyDark1/esp32_p4_jc4880p433c_bsp
 * (licença Apache 2.0). Baixei o repositório inteiro e extraí esses
 * valores de include/bsp/esp-bsp.h, Kconfig e src/bsp_display.c —
 * não é mais chute.
 */

#pragma once

#include "driver/gpio.h"

/* ---------------------------------------------------------------------
 * I2C compartilhado (touch GT911 + câmera OV02C10)           [CONFIRMADO]
 * --------------------------------------------------------------------- */
#define BOARD_I2C_PORT        0
#define BOARD_I2C_SDA_GPIO    7
#define BOARD_I2C_SCL_GPIO    8
#define BOARD_I2C_CLK_HZ      400000

#define BOARD_GT911_ADDR_1    0x14
#define BOARD_GT911_ADDR_2    0x5D

/* Confirmados no esquemático oficial da GUITION (JC-ESP32P4-M3).*/
#define BOARD_TOUCH_RST_GPIO   GPIO_NUM_22
#define BOARD_TOUCH_INT_GPIO   GPIO_NUM_21

/* ---------------------------------------------------------------------
 * Painel MIPI-DSI (ST7701S)                                   [CONFIRMADO]
 * --------------------------------------------------------------------- */
#define BOARD_MIPI_DSI_LANE_NUM        2
#define BOARD_MIPI_DSI_LANE_BITRATE_MBPS  500

/* Resolução NATIVA do painel é retrato (480x800). O gabinete do PDA vai
 * usar a tela deitada (800x480), então a rotação para paisagem deve ser
 * feita via software (sw_rotate). */
#define BOARD_LCD_H_RES_NATIVE   480
#define BOARD_LCD_V_RES_NATIVE   800

/* DPI clock confirmado (34MHz ~ 60Hz pra esse painel).*/
#define BOARD_LCD_DPI_CLOCK_MHZ   34
#define BOARD_LCD_NUM_FB          2   /* double buffer, igual ao BSP de referência */

#define BOARD_MIPI_DSI_PHY_LDO_CHAN   3
#define BOARD_MIPI_DSI_PHY_LDO_MV     2500

/* ---------------------------------------------------------------------
 * Reset / Backlight do painel                                 [CONFIRMADO]
 * --------------------------------------------------------------------- */
#define BOARD_LCD_RST_GPIO     GPIO_NUM_5

/* Backlight é PWM (LEDC), não um GPIO digital simples. */
#define BOARD_LCD_BL_GPIO           GPIO_NUM_23
#define BOARD_LCD_BL_LEDC_TIMER     LEDC_TIMER_1
#define BOARD_LCD_BL_LEDC_CHANNEL   LEDC_CHANNEL_1
#define BOARD_LCD_BL_PWM_FREQ_HZ    20000

/* ---------------------------------------------------------------------
 * WiFi/BT via ESP32-C6 coprocessador (SDIO)                  [CONFIRMADO]
 * --------------------------------------------------------------------- */
/* Fonte: repositório de field-notes especificamente sobre essa placa
 * (github.com/ultramcu/guition-jc4880p443c-i-w) — não verificado por nós
 * ainda em hardware real, mas é a fonte mais específica que encontramos.
 *
 * ATENÇÃO — antes de escrever qualquer código de WiFi: essa placa sai de
 * fábrica com o firmware do C6 na versão 2.3.0, incompatível com host
 * moderno (que espera ~2.12+). Sintoma se não corrigir: WiFi associa e
 * cai em loop (ASSOC_LEAVE / "Restarting adapter"). É preciso reflashar
 * o C6 ANTES de testar qualquer coisa — ver README, seção WiFi/C6. */
#define BOARD_C6_SDIO_CLK_GPIO   GPIO_NUM_18
#define BOARD_C6_SDIO_CMD_GPIO   GPIO_NUM_19
#define BOARD_C6_SDIO_D0_GPIO    GPIO_NUM_14
#define BOARD_C6_SDIO_D1_GPIO    GPIO_NUM_15
#define BOARD_C6_SDIO_D2_GPIO    GPIO_NUM_16
#define BOARD_C6_SDIO_D3_GPIO    GPIO_NUM_17
#define BOARD_C6_SDIO_FREQ_KHZ   40000

#define BOARD_C6_RESET_GPIO      GPIO_NUM_54  /* ativo em nível ALTO */

/* Pinos de UART0 do C6, expostos no header Expand-IO (JP1) — só usados
 * pra reflashar o firmware do C6 via adaptador USB-TTL externo, não
 * usados em operação normal. */
#define BOARD_C6_UART_TXD_LABEL "C6_U0RXD"  /* no JP1 — TX do adaptador vai aqui */
#define BOARD_C6_UART_RXD_LABEL "C6_U0TXD"  /* no JP1 — RX do adaptador vai aqui */
#define BOARD_C6_BOOT_LABEL     "C6_IO9"    /* puxar pra GND durante o reset entra em modo download */
#define BOARD_C6_CHIP_PU_LABEL  "C6_CHIP_PU"

/* ---------------------------------------------------------------------
 * USB Host (teclado)                                          [CONFIRMADO]
 * --------------------------------------------------------------------- */
/* Confirmado via schematic oficial do fabricante: a placa tem duas
 * portas USB-C fisicamente distintas, ligadas a periféricos diferentes
 * do ESP32-P4:
 *   - "Full Speed USB" (bloco USB2 no schematic) -> USB-Serial-JTAG.
 *     Essa é a porta de flash/monitor (idf.py flash monitor). NÃO serve
 *     pra Host mode.
 *   - "High Speed USB" (bloco USB3, sinais ESP_USB_P/ESP_USB_N) ->
 *     USB-OTG 2.0 HS nativo. Essa é a porta certa pro teclado. */

 /* ---------------------------------------------------------------------
 * Cartão MicroSD (SDMMC 4-bit) + LDO de alimentação          [CONFIRMADO]
 * --------------------------------------------------------------------- */
#define BOARD_SD_LDO_CHAN    4
#define BOARD_SD_LDO_MV      3300  /* 3.3V padrão para MicroSD */

#define BOARD_SD_CLK_GPIO    GPIO_NUM_43
#define BOARD_SD_CMD_GPIO    GPIO_NUM_44
#define BOARD_SD_D0_GPIO     GPIO_NUM_39
#define BOARD_SD_D1_GPIO     GPIO_NUM_40
#define BOARD_SD_D2_GPIO     GPIO_NUM_46
#define BOARD_SD_D3_GPIO     GPIO_NUM_45
