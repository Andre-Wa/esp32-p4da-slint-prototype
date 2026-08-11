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

# Compila ui/app_ui.slint -> C++ e linka no componente main.
# Essa função (slint_target_sources) é fornecida pelo componente
# slint/slint puxado via idf_component.yml.
slint_target_sources(${COMPONENT_LIB} ui/app_ui.slint)
