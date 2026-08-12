idf_component_register(
    SRCS
        "main.cpp"
        "display_init.c"
        "touch_init.c"
        "usb_hid_keyboard.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_lcd
        usb
        driver
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_ledc
        nvs_flash
        esp_psram
)

# CMP0116 controla como o Ninja interpreta o DEPFILE gerado pelo
# add_custom_command() usado internamente pelo slint_target_sources().
# Sem forçar isso pra NEW, o Ninja pode achar pra sempre que o arquivo
# gerado a partir do .slint está desatualizado — o que causa um loop
# infinito de "Re-running CMake" até o Ninja desistir com
# "manifest 'build.ninja' still dirty after 100 tries" (mensagem enganosa,
# não tem nada a ver com relógio do sistema nesse caso).
cmake_policy(SET CMP0116 NEW)

# Compila ui/app_ui.slint -> C++ e linka no componente main.
slint_target_sources(${COMPONENT_LIB} ui/app_ui.slint)
