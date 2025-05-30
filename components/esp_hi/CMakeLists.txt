idf_component_register(
    SRCS "esp_hi.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    PRIV_REQUIRES
        "esp_lcd"
    REQUIRES
        "driver"
        "spiffs"
        "esp_codec_dev"
)