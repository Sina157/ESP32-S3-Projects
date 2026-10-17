# ESP32-S3 Wi-Fi Audio Player

This project demonstrates how to stream and play MP3 audio over a local Wi-Fi network using an ESP32-S3 microcontroller and an I2S audio amplifier.

The ESP32-S3 connects to a Wi-Fi network, retrieves an MP3 file from a local HTTP server, decodes the audio stream, and outputs digital audio through the I2S interface.

## Features

- **Wi-Fi Connectivity:** Connects the ESP32-S3 to a wireless network.
- **Network Audio Streaming:** Retrieves an MP3 file from a local HTTP server.
- **MP3 Playback:** Uses the `Audio` library to handle audio streaming and playback.
- **I2S Digital Audio Output:** Transmits digital audio data to an external I2S-compatible audio amplifier.
- **Volume Control:** Configures the playback volume through the audio library.
- **Real-Time Playback:** Uses `audio.loop()` to continuously process incoming audio data and maintain playback.

## Hardware Requirements

- ESP32-S3 development board (ESP32-S3-WROOM-1-N16R8)
- MAX98357A I2S audio amplifier
- Compatible speaker
- Wi-Fi router or access point
- Computer or another device hosting the MP3 file on a local HTTP server

## Wiring

The GPIO configuration used in this project is:

| MAX98357A Pin | ESP32-S3 Pin |
|---|---|
| DIN | GPIO 9 |
| BCLK | GPIO 10 |
| LRC / LRCLK | GPIO 11 |
| VIN | Compatible power supply |
| GND | GND |

Connect the speaker to the MAX98357A speaker output terminals. Ensure that the amplifier's power supply matches its specifications.

## Software Requirements

- Arduino IDE
- ESP32 Arduino Core
- `WiFi.h`
- `Audio.h` library

## How It Works

1. The ESP32-S3 connects to the configured Wi-Fi network.
2. After establishing the connection, the program initializes the I2S audio output pins.
3. The playback volume is set to level 21.
4. The audio library connects to the MP3 URL hosted on the local network.
5. The library receives and decodes the MP3 stream.
6. Decoded audio is transmitted through I2S to the MAX98357A amplifier for playback.

## Configuration

### Wi-Fi Settings

Update the Wi-Fi credentials in the source code:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### MP3 Stream URL

Configure the address of the MP3 file hosted on your local network:

```cpp
audio.connecttohost(
  "http://192.168.1.102:8000/tone-test.mp3"
);
```

Replace the IP address, port, and filename with the actual address of your HTTP server.

The server and ESP32-S3 must be able to communicate over the network, and the MP3 file must be accessible through the specified URL.

### I2S Pin Configuration

```cpp
#define I2S_DOUT  9
#define I2S_BCLK  10
#define I2S_LRC   11
```

These GPIO assignments define the digital audio connection between the ESP32-S3 and the amplifier.

## Applications

This project provides a foundation for developing:

- Network-connected audio players
- Internet radio and local audio streaming devices
- Voice playback systems
- Audio notification systems
- Embedded audio applications integrated with sensors or other IoT devices

## Project Goal

The goal of this project is to gain practical experience with Wi-Fi networking, HTTP-based media streaming, MP3 decoding, I2S communication, and external audio amplification on the ESP32-S3.

It demonstrates how a microcontroller can retrieve digital audio from another device on the network and play it through external audio hardware.

## Demo

Watch the ESP32-S3 Wi-Fi Audio Player in action:

[▶ Watch the project demonstration on YouTube Shorts](https://www.youtube.com/shorts/8rb5SQG5J4Q)

The video demonstrates the project running on real hardware.
