idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_hi)

set(Drive_A "../gif")

spiffs_create_partition_assets(
    assets
    ${Drive_A}
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".png,.jpg"
    MMAP_SUPPORT_QOI
    MMAP_SUPPORT_SQOI
    MMAP_SPLIT_HEIGHT 4)
                    

spiffs_create_partition_assets(
    audio
    "../audio"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".wav"
)