#include <WiFi.h>
#include "Audio.h"

const char* ssid = "WIFE_NAME";
const char* password = "PASSWORD";

#define I2S_DOUT  9
#define I2S_BCLK  10
#define I2S_LRC   11

Audio audio;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");

  audio.setPinout(
    I2S_BCLK,
    I2S_LRC,
    I2S_DOUT
  );

  audio.setVolume(21);

  Serial.println("Playing MP3...");

  audio.connecttohost(
    "http://192.168.1.102:8000/tone-test.mp3"
  );
}

void loop() {
  audio.loop();
}
