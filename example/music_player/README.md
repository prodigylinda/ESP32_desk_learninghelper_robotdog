# Music Player Example

This example demonstrates a local music player on the ESP-HI development board, supporting loop playback of multiple audio files. Users can control playback, pause, and switch to the next track using a single button.

## Features

- Supports loop playback of multiple audio files (requires flashing the audio partition)
- Single button control:
  - When idle: Play the current audio
  - During playback: Pause playback
  - When paused: Switch to the next track and play
- Automatically pauses after playback completes
- Audio parameters: 44100Hz sample rate, mono channel, 16-bit
- Maximum volume

## Hardware Requirements

- ESP-HI development board
- On-board speaker
- BOOT button (for playback control)
- USB cable (for power and programming)

## Usage

1. Prepare audio files and generate an audio partition image using the appropriate tool, then flash it to the designated partition
2. Connect the ESP-HI development board to your computer
3. Compile and flash the example as follows:
   ```bash
   idf.py build
   idf.py -p (PORT) flash monitor
   ```
4. Press the BOOT button to control playback/pause/track switching

## Notes

- Ensure the audio partition is correctly flashed with audio files; otherwise, playback will not work
- The number and format of audio files must match the program configuration
- If no sound is heard during playback, check the speaker connection and volume settings

## Related Files

- main/main.c: Main program entry and button/audio playback logic
- audio/: Directory containing audio partition files

For customizing audio or functionality, refer to the source code and comments.