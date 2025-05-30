# Audio Loop Recording and Playback Example

This example demonstrates a simple audio recording and playback functionality using ESP-Hi. It allows users to record audio by pressing a button and play it back when the button is released.

## Features

- Press and hold to record audio
- Release button to stop recording and play back
- Maximum recording time: 3 seconds
- Default sample rate: 16kHz
- Audio format: 16-bit PCM, mono channel
- Real-time recording and playback

## Hardware Requirements

- ESP-Hi Board

## Usage

1. Press and hold the BOOT button to start recording
2. Speak into the microphone while holding the button
3. Release the button to stop recording and play back the recorded audio
4. The recorded audio will play through the built-in speaker


## Building and Running

1. Set up ESP-IDF environment
   ```
2. Build the project:
   ```bash
   idf.py build
   ```
3. Flash the project:
   ```bash
   idf.py -p (PORT) flash monitor
   ```

## Troubleshooting

1. If no sound is heard:
   - Check if the speaker is properly connected
   - Verify the volume level in the code
   - Check if the audio codec is properly initialized

2. If recording quality is poor:
   - Check if the microphone is properly connected
   - Verify the sample rate and bit depth settings
   - Check for any interference sources near the microphone
