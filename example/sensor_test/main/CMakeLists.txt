idf_component_register(SRCS "sensor_test.cpp" "m5_dlight.c" "sht20.c" "paj7620u2.c"
                    INCLUDE_DIRS "."
                    REQUIRES i2c_bus BME688 esp_timer BSEC2) 