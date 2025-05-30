# Sensor Test Example

This example demonstrates how to use various sensors with ESP32, including:
- BME688 (Temperature, Humidity, Pressure and Gas sensor)
- PAJ7620U2 (Gesture recognition sensor)
- SHT20 (Temperature and Humidity sensor)
- M5_DLIGHT (Light sensor)

## Hardware Requirements

- ESP32 development board
- BME688 sensor
- PAJ7620U2 sensor
- SHT20 sensor
- M5_DLIGHT sensor
- I2C connections:
  - SCL: GPIO 1
  - SDA: GPIO 2

## Software Dependencies

- ESP-IDF (v4.4 or later)
- BSEC2 library for BME688
- I2C bus driver

## Building and Running

1. Set up ESP-IDF environment:
```bash
. $IDF_PATH/export.sh
```

2. Configure the project:
```bash
idf.py menuconfig
```

3. Build the project:
```bash
idf.py build
```

4. Flash the project:
```bash
idf.py -p (PORT) flash
```

5. Monitor the output:
```bash
idf.py -p (PORT) monitor
```

## Features

- BME688 sensor:
  - Temperature measurement
  - Humidity measurement
  - Pressure measurement
  - Gas resistance measurement
  - IAQ (Indoor Air Quality) calculation using BSEC2 library

- PAJ7620U2 sensor:
  - Gesture recognition
  - 9 different gestures supported

- SHT20 sensor:
  - Temperature measurement
  - Humidity measurement

- M5_DLIGHT sensor:
  - Ambient light measurement

## Notes

- The I2C bus is configured to run at 400kHz
- BME688 uses BSEC2 library for advanced sensor fusion and IAQ calculation
- All sensor data is logged using ESP logging system 