
#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>
#include <esp_heap_caps.h>

// ==================== INMP441 ====================
#define MIC_BCLK 4
#define MIC_WS   5
#define MIC_SD   6

// ==================== MAX98357A ====================
#define SPK_BCLK 10
#define SPK_LRC  11
#define SPK_DIN  9

// ==================== Audio settings ====================
#define SAMPLE_RATE 16000
#define BLOCK_SIZE 256
#define MAX_RECORD_SECONDS 30

// Voice detection sensitivity
#define MIN_START_THRESHOLD 350
#define MIN_SILENCE_THRESHOLD 180
#define LOUD_BLOCKS_REQUIRED 2

// About 2 seconds of silence
#define SILENCE_BLOCKS 125

// Playback gain
#define PLAYBACK_GAIN 20

// ==================== Buffers ====================
int32_t rawBuffer[BLOCK_SIZE];
int16_t *audioBuffer = nullptr;

size_t recordedSamples = 0;
size_t maxSamples = 0;

float noiseFloor = 0;
float startThreshold = MIN_START_THRESHOLD;
float silenceThreshold = MIN_SILENCE_THRESHOLD;

// ==================================================
// Initialize microphone
// ==================================================
bool setupMic() {
  i2s_driver_uninstall(I2S_NUM_0);

  i2s_config_t config = {};
  config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = SAMPLE_RATE;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 8;
  config.dma_buf_len = BLOCK_SIZE;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
  config.fixed_mclk = 0;

  esp_err_t result =
    i2s_driver_install(I2S_NUM_0, &config, 0, NULL);

  if (result != ESP_OK) {
    Serial.printf("Mic driver error: %d\n", result);
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.bck_io_num = MIC_BCLK;
  pins.ws_io_num = MIC_WS;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = MIC_SD;

  result = i2s_set_pin(I2S_NUM_0, &pins);

  if (result != ESP_OK) {
    Serial.printf("Mic pin error: %d\n", result);
    i2s_driver_uninstall(I2S_NUM_0);
    return false;
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
  delay(100);

  Serial.println("Microphone ready");
  return true;
}

// ==================================================
// Read microphone samples
// ==================================================
bool readMicBlock() {
  size_t bytesRead = 0;

  esp_err_t result = i2s_read(
    I2S_NUM_0,
    rawBuffer,
    sizeof(rawBuffer),
    &bytesRead,
    portMAX_DELAY
  );

  return result == ESP_OK &&
         bytesRead >= sizeof(rawBuffer);
}

// ==================================================
// Calculate RMS and remove DC offset
// ==================================================
float getRMS() {
  double mean = 0;
  double sum = 0;

  for (int i = 0; i < BLOCK_SIZE; i++) {
    mean += (double)(rawBuffer[i] >> 14);
  }

  mean /= BLOCK_SIZE;

  for (int i = 0; i < BLOCK_SIZE; i++) {
    double sample = (double)(rawBuffer[i] >> 14) - mean;
    sum += sample * sample;
  }

  return sqrt(sum / BLOCK_SIZE);
}

// ==================================================
// Measure background noise
// ==================================================
bool calibrateNoise() {
  Serial.println();
  Serial.println("Calibrating noise. Stay quiet...");

  double total = 0;
  const int blocks = 100;

  for (int i = 0; i < blocks; i++) {
    if (!readMicBlock()) {
      Serial.println("Mic read error");
      return false;
    }

    total += getRMS();
    delay(5);
  }

  noiseFloor = total / blocks;

  startThreshold = max(
    (float)MIN_START_THRESHOLD,
    noiseFloor * 1.35f
  );

  silenceThreshold = max(
    (float)MIN_SILENCE_THRESHOLD,
    noiseFloor * 1.15f
  );

  Serial.printf("Background RMS: %.0f\n", noiseFloor);
  Serial.printf("Voice threshold: %.0f\n", startThreshold);
  Serial.printf("Silence threshold: %.0f\n", silenceThreshold);
  Serial.println("Listening for voice...");

  return true;
}

// ==================================================
// Record voice, up to 30 seconds
// ==================================================
bool recordWhenSpeaking() {
  recordedSamples = 0;

  int loudBlocks = 0;
  int silentBlocks = 0;
  bool recording = false;

  Serial.println("Waiting for voice...");

  while (recordedSamples < maxSamples) {
    if (!readMicBlock()) {
      Serial.println("Mic read error");
      break;
    }

    float rms = getRMS();

    if (!recording) {
      Serial.printf("Ambient RMS: %.0f\n", rms);

      if (rms > startThreshold) {
        loudBlocks++;
      } else {
        loudBlocks = 0;
      }

      if (loudBlocks < LOUD_BLOCKS_REQUIRED) {
        continue;
      }

      recording = true;
      silentBlocks = 0;
      Serial.println("Voice detected! Recording...");
    }

    for (int i = 0; i < BLOCK_SIZE; i++) {
      if (recordedSamples >= maxSamples) {
        break;
      }

      // Convert microphone data to a 16-bit-scale sample.
      int32_t sample = rawBuffer[i] >> 14;

      // Requested x20 gain, with clipping protection.
      sample *= PLAYBACK_GAIN;

      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;

      audioBuffer[recordedSamples++] = (int16_t)sample;
    }

    if (rms < silenceThreshold) {
      silentBlocks++;
    } else {
      silentBlocks = 0;
    }

    if (silentBlocks >= SILENCE_BLOCKS) {
      Serial.println("Silence detected. Stopping recording.");
      break;
    }
  }

  if (!recording || recordedSamples == 0) {
    Serial.println("No voice recording captured.");
    return false;
  }

  Serial.printf(
    "Recorded %.2f seconds\n",
    (float)recordedSamples / SAMPLE_RATE
  );

  return true;
}

// ==================================================
// Initialize speaker
// ==================================================
bool setupSpeaker() {
  i2s_driver_uninstall(I2S_NUM_0);

  i2s_config_t config = {};
  config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = SAMPLE_RATE;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 8;
  config.dma_buf_len = BLOCK_SIZE;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;
  config.fixed_mclk = 0;

  esp_err_t result =
    i2s_driver_install(I2S_NUM_0, &config, 0, NULL);

  if (result != ESP_OK) {
    Serial.printf("Speaker driver error: %d\n", result);
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.bck_io_num = SPK_BCLK;
  pins.ws_io_num = SPK_LRC;
  pins.data_out_num = SPK_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  result = i2s_set_pin(I2S_NUM_0, &pins);

  if (result != ESP_OK) {
    Serial.printf("Speaker pin error: %d\n", result);
    i2s_driver_uninstall(I2S_NUM_0);
    return false;
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
  delay(100);

  Serial.println("Speaker ready");
  return true;
}

// ==================================================
// Play recorded audio
// ==================================================
void playAudio() {
  if (recordedSamples == 0) {
    return;
  }

  Serial.println("Playing audio...");

  if (!setupSpeaker()) {
    return;
  }

  size_t offset = 0;

  while (offset < recordedSamples) {
    size_t count = min(
      (size_t)BLOCK_SIZE,
      recordedSamples - offset
    );

    size_t bytesWritten = 0;

    esp_err_t result = i2s_write(
      I2S_NUM_0,
      &audioBuffer[offset],
      count * sizeof(int16_t),
      &bytesWritten,
      portMAX_DELAY
    );

    if (result != ESP_OK || bytesWritten == 0) {
      Serial.println("Speaker write error");
      break;
    }

    offset += bytesWritten / sizeof(int16_t);
  }

  delay(100);
  i2s_zero_dma_buffer(I2S_NUM_0);
  i2s_driver_uninstall(I2S_NUM_0);

  Serial.println("Playback finished");
}

// ==================================================
// Setup
// ==================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-S3 Voice Recorder");

  // Allocate the 30-second audio buffer in PSRAM.
  if (!psramFound()) {
    Serial.println("ERROR: PSRAM not found!");
    while (true) {
      delay(1000);
    }
  }

  maxSamples = (size_t)SAMPLE_RATE * MAX_RECORD_SECONDS;

  audioBuffer = (int16_t *)ps_malloc(
    maxSamples * sizeof(int16_t)
  );

  if (audioBuffer == nullptr) {
    Serial.println("ERROR: PSRAM allocation failed!");
    while (true) {
      delay(1000);
    }
  }

  Serial.printf(
    "PSRAM buffer allocated: %u bytes\n",
    (unsigned int)(maxSamples * sizeof(int16_t))
  );

  if (!setupMic()) {
    Serial.println("Microphone initialization failed");
    while (true) {
      delay(1000);
    }
  }
}

// ==================================================
// Main loop
// ==================================================
void loop() {
  if (!setupMic()) {
    delay(1000);
    return;
  }

  if (!calibrateNoise()) {
    delay(1000);
    return;
  }

  if (recordWhenSpeaking()) {
    delay(300);
    playAudio();
  }

  delay(500);
}
