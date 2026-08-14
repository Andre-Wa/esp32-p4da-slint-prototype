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

/* Confirmados no esquemático oficial da GUITION (JC-ESP32P4-M3). Ver
 * nota em touch_init.c sobre o que rst_gpio_num de fato melhora. */
#define BOARD_TOUCH_RST_GPIO   GPIO_NUM_22
#define BOARD_TOUCH_INT_GPIO   GPIO_NUM_20

/* Confirmados no esquemático oficial da GUITION (JC-ESP32P4-M3):
 * reset e interrupt dedicados do GT911, hoje não utilizados (o driver
 * funciona via polling puro sem eles, mas conectá-los deixa o touch
 * mais robusto — reset garante estado limpo no boot). */
#define BOARD_TOUCH_RST_GPIO   GPIO_NUM_22
#define BOARD_TOUCH_INT_GPIO   GPIO_NUM_20

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

/* DPI clock confirmado (34MHz ~ 60Hz pra esse painel). Meu chute
 * original (27MHz) provavelmente também não ajudava. */
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
