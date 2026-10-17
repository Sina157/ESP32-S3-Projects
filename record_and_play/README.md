# ESP32-S3 Voice Recorder and Playback

A voice-activated audio recording and playback project built with the **ESP32-S3**, **INMP441 I2S microphone**, and **MAX98357A I2S amplifier**.

The system continuously monitors ambient sound, detects when someone starts speaking, records the audio, and plays it back through a speaker after detecting approximately two seconds of silence.

The recorded audio is stored in PSRAM, allowing recordings of up to 30 seconds without consuming the ESP32-S3's limited internal RAM.

## Features

- **Voice activation:** Automatically detects speech using RMS (Root Mean Square) audio analysis.
- **Background noise calibration:** Measures ambient noise and adjusts the voice detection thresholds.
- **Automatic recording:** Starts recording after detecting consecutive loud audio blocks.
- **Silence detection:** Stops recording after approximately two seconds of continuous silence.
- **Audio playback:** Plays the recorded voice through the MAX98357A amplifier.
- **PSRAM storage:** Allocates a large audio buffer in external PSRAM.
- **Clipping protection:** Limits amplified samples to the valid 16-bit PCM range.
- **Configurable settings:** Allows adjustment of recording duration, detection sensitivity, sample rate, and playback gain.

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32-S3 development board with PSRAM | Main controller and audio processing |
| INMP441 I2S microphone | Captures voice and ambient sound |
| MAX98357A I2S amplifier | Converts digital audio into amplified speaker output |
| Compatible speaker | Plays back recorded audio |
| Jumper wires | Connects the modules |

## Wiring

### INMP441 Microphone

| INMP441 Pin | ESP32-S3 GPIO |
|---|---|
| SCK / BCLK | GPIO 4 |
| WS / LRCL | GPIO 5 |
| SD | GPIO 6 |
| VDD | 3.3V |
| GND | GND |

### MAX98357A Amplifier

| MAX98357A Pin | ESP32-S3 GPIO |
|---|---|
| BCLK | GPIO 10 |
| LRC / LCLK | GPIO 11 |
| DIN | GPIO 9 |
| VIN | Compatible power supply |
| GND | GND |

Connect the speaker to the amplifier's speaker output terminals, following the module's specifications.

**Important:** The microphone and amplifier use separate GPIO pins for their I2S signals. The firmware switches the I2S driver between receive and transmit modes because both devices use `I2S_NUM_0`.

Check your particular modules' power requirements before connecting them. Do not connect the amplifier's speaker output directly to an ESP32 GPIO or ground.

## Software Requirements

- Arduino IDE
- ESP32 board support package for Arduino
- ESP32-S3 board with working PSRAM
- ESP32 Arduino I2S driver (`driver/i2s.h`)

The sketch uses the legacy I2S driver API. Compatibility may depend on the installed ESP32 Arduino core version.

## Configuration

The main settings can be changed at the beginning of the sketch.

| Setting | Default | Description |
|---|---:|---|
| `SAMPLE_RATE` | 16000 Hz | Audio sampling frequency |
| `BLOCK_SIZE` | 256 samples | Number of samples processed per block |
| `MAX_RECORD_SECONDS` | 30 seconds | Maximum recording duration |
| `MIN_START_THRESHOLD` | 350 | Minimum voice activation threshold |
| `MIN_SILENCE_THRESHOLD` | 180 | Minimum silence detection threshold |
| `LOUD_BLOCKS_REQUIRED` | 2 | Consecutive loud blocks required to start recording |
| `SILENCE_BLOCKS` | 125 | Consecutive quiet blocks required to stop recording |
| `PLAYBACK_GAIN` | 20 | Digital sample multiplication factor |

The gain setting is a digital multiplier, not a guaranteed 20× increase in perceived loudness. Excessive gain can cause clipping and distortion.

## How It Works

1. **Initialize PSRAM:** Allocates an audio buffer large enough for a 30-second recording.
2. **Initialize the microphone:** Configures I2S in receive mode.
3. **Calibrate background noise:** Samples the microphone and calculates the average RMS level.
4. **Wait for speech:** Monitors incoming audio until the configured activation threshold is exceeded for two consecutive blocks.
5. **Record audio:** Converts the microphone's 32-bit I2S samples to 16-bit audio samples and stores them in PSRAM.
6. **Detect silence:** Stops recording when the configured number of quiet blocks is reached, or when the maximum recording duration is reached.
7. **Switch to playback mode:** Reconfigures I2S for transmission and initializes the MAX98357A.
8. **Play the recording:** Sends the stored audio samples to the amplifier.
9. **Repeat:** Returns to microphone mode and starts another detection cycle.

## Memory Usage

At a sampling rate of 16,000 Hz, using 16-bit mono audio:

- Samples per second: 16,000
- Storage per sample: 2 bytes
- Maximum recording duration: 30 seconds
- Audio buffer size: 960,000 bytes, approximately 938 KiB

The large audio buffer is allocated in PSRAM using `ps_malloc()`, reducing pressure on internal RAM.

The microphone's temporary input buffer and the I2S DMA buffers use additional memory.

## Serial Monitor

Set the Serial Monitor baud rate to **115200**.

During operation, the program reports messages such as:

- `Microphone ready`
- `Calibrating noise. Stay quiet...`
- `Background RMS: ...`
- `Listening for voice...`
- `Voice detected! Recording...`
- `Silence detected. Stopping recording.`
- `Playing audio...`
- `Playback finished`

These messages help with debugging microphone initialization, noise thresholds, recording, and playback.

## Limitations and Possible Improvements

- **Noise sensitivity:** Loud background noise may trigger recording. The thresholds may need adjustment for different environments.
- **Fixed silence timeout:** The current silence timeout is based on 125 audio blocks. At 16 kHz with 256 samples per block, this represents approximately two seconds of audio.
- **Digital gain:** Multiplying samples can amplify noise and cause clipping. Better gain control or normalization could improve sound quality.
- **Mono audio:** The project records and plays back mono audio.
- **Legacy I2S API:** Newer ESP32 Arduino core versions may require migration to the newer I2S driver.
- **No persistent storage:** Recordings are kept in RAM and are lost when the device restarts or power is removed.
- **No speech recognition:** The project detects sound levels, not spoken words. It does not yet use AI or speech-to-text.

## Future Improvements

- Add speech recognition using an online API or an offline speech model.
- Send recorded audio to an AI service for voice interaction.
- Add noise filtering and automatic gain control.
- Store recordings on an SD card.
- Add configurable recording and silence thresholds.
- Implement a wake-word detection system.

## Project Goal

This project explores embedded audio processing with the ESP32-S3, digital microphones, I2S communication, PSRAM memory management, voice activity detection, and digital audio playback.

It provides a foundation for more advanced projects such as voice-controlled robots, AI assistants, and interactive embedded systems.
