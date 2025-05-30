idf_component_register(
    SRCS 
        "bsec_esp_interface.cpp"
        "./Bosch-BSEC2-Library/src/bsec2.cpp"
        "./Bosch-BME68x-Library/src/bme68xLibrary.cpp"
    INCLUDE_DIRS 
        "."
        "Bosch-BSEC2-Library/src"
        "Bosch-BME68x-Library/src"
    REQUIRES 
        driver
        i2c_bus
        esp_rom
        esp_common
        esp_system
)


add_prebuilt_library(libalgobsec "Bosch-BSEC2-Library/src/esp32c3/libalgobsec.a"
             PRIV_REQUIRES nvs_flash json bt)
target_link_libraries(${COMPONENT_LIB} libalgobsec)