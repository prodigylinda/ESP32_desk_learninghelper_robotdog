idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_hi)

spiffs_create_partition_assets(
    audio
    "../audio"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".wav"
)