idf_component_register(SRCS "app_https.c" "app_wifi.c" "app_main.c" "app_servo_control.c"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
