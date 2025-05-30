# LCD Audio Example

This example demonstrates the high-speed image refresh capability of ESP32-C3, combined with audio playback functionality. It showcases how ESP32-C3 can efficiently handle both display and audio processing simultaneously.

## Features

- High-speed image refresh on LCD display
- Synchronized audio playback
- Support for multiple image formats
- Efficient memory management
- Real-time display and audio processing

## Hardware Requirements

- ESP-Hi Board

## Project Structure

```
lcd_audio/
├── main/           # Main application code
├── audio/          # Audio related files
├── gif/            # Image resources
└── CMakeLists.txt  # Build configuration
```

## Building and Running

1. Set up ESP-IDF environment
2. Build the project:
   ```bash
   idf.py build
   ```
3. Flash and monitor:
   ```bash
   idf.py -p (PORT) flash monitor
   ```

