# Wake Word Test Example

This example demonstrates how to implement wake word detection on ESP32-C3 using ESP-IDF. The project includes audio playback, LED control, and wake word detection functionality.

## Features

- Wake word detection using ESP-WakeNet
- Audio playback with visual feedback
- LED strip control
- Button control for user interaction
- LVGL-based display interface
- Memory-mapped asset management for audio and GIF files

## Hardware Requirements

- ESP32-C3 development board
- 4MB Flash
- LED strip (4 LEDs)
- Speaker/audio output device
- Button for user input

## Software Requirements

- ESP-IDF v5.4.1 or later
- ESP-WakeNet model
- LVGL library
- ESP-BSP components

## Project Structure

```
wake_word_test/
├── audio/           # Audio assets
├── gif/            # Animation assets
├── main/           # Main application code
│   ├── main.cpp    # Main application logic
│   ├── mmap_generate_audio.h
│   └── mmap_generate_gif.h
├── managed_components/  # ESP-IDF managed components
└── partitions.csv  # Custom partition table
```

## Configuration

The project uses the following default configurations:

- Target: ESP32-C3
- Flash size: 4MB
- FreeRTOS tick rate: 1000Hz
- Custom partition table
- LVGL configuration for display
- ESP-WakeNet model: WN9S_HILEXIN

## Building and Running

1. Set up ESP-IDF environment:
```bash
. $IDF_PATH/export.sh
```

2. Configure the project:
```bash
idf.py set-target esp32c3
idf.py menuconfig
```

3. Build and flash:
```bash
idf.py build
idf.py -p (PORT) flash
```

4. Monitor the output:
```bash
idf.py -p (PORT) monitor
```

## Usage

1. After flashing, the device will initialize the wake word detection system
2. Press the button to start recording
3. Speak the wake word to trigger the detection
4. Upon successful detection:
   - LED strip will show visual feedback
   - Audio feedback will play
   - Display will show animation

## Notes

- The wake word detection uses the WN9S_HILEXIN model
- Audio playback supports both one-shot and continuous modes
- LED strip provides visual feedback for system status
- The system uses memory-mapped assets for efficient resource management

## Troubleshooting

If you encounter any issues:

1. Check the serial monitor for error messages
2. Verify the audio input/output connections
3. Ensure the wake word model is properly flashed
4. Check the LED strip connections

## License

This example is licensed under the CC0-1.0 License.
