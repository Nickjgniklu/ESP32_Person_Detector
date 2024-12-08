#include "save_task.h"
#include "JpegImage.h"
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define TAG "SAVE_TASK"
#define SD_CARD_PIN 21

typedef struct
{
  QueueHandle_t jpegQueue;
} AITaskParams_t;

// SD card write file
void writeFile(fs::FS &fs, const char *path, uint8_t *data, size_t len)
{
  ESP_LOGI(TAG, "Writing file: %s", path);
  unsigned long start = millis();
  unsigned long openStart = millis();
  File file = fs.open(path, FILE_WRITE);
  unsigned long openEnd = millis();
  if (!file)
  {
    ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
    return;
  }
  ESP_LOGI(TAG, "Time to open file: %lu ms", openEnd - openStart);

  size_t chunkSize = 512;
  size_t written = 0;
  while (written < len)
  {
    unsigned long chunkStart = millis();
    size_t toWrite = (len - written) < chunkSize ? (len - written) : chunkSize;
    if (file.write(data + written, toWrite) != toWrite)
    {
      ESP_LOGE(TAG, "Write failed at chunk starting at %d", written);
      file.close();
      return;
    }
    unsigned long chunkEnd = millis();
    ESP_LOGI(TAG, "Time to write chunk: %lu ms", chunkEnd - chunkStart);
    written += toWrite;
  }

  unsigned long end = millis();
  ESP_LOGI(TAG, "File of size %d written in %lu ms", len, end - start);
  file.close();
}

void concatFileDump(fs::File &file, uint8_t *data, size_t len)
{
  if (!file)
  {
    ESP_LOGE(TAG, "file not open");
    return;
  }
  file.seek(file.size());

  unsigned long start = millis();
  size_t written = file.write(data, len);
  file.flush();
  unsigned long end = millis();

  if (written != len)
  {
    ESP_LOGE(TAG, "Failed to append data to file");
  }
  else
  {
    ESP_LOGI(TAG, "Appended %d bytes to file in %lu ms", written, end - start);
  }
}

u64_t countFilesInRoot(fs::FS &fs)
{
  File root = fs.open("/");
  if (!root)
  {
    ESP_LOGI(TAG, "Failed to open directory");
    return 0;
  }
  if (!root.isDirectory())
  {
    ESP_LOGI(TAG, "Not a directory");
    return 0;
  }

  File file = root.openNextFile();
  u64_t count = 0;
  while (file)
  {
    if (!file.isDirectory())
    {
      count++;
    }
    file = root.openNextFile();
  }
  ESP_LOGI(TAG, "File count: %d", count);
  return count;
}

bool setupSdCard()
{
  // Initialize SD card
  if (!SD.begin(SD_CARD_PIN, SPI, 40'000'000u))
  {
    ESP_LOGI(TAG, "Card Mount Failed");
    return false;
  }
  uint8_t cardType = SD.cardType();

  // Determine if the type of SD card is available
  if (cardType == CARD_NONE)
  {
    ESP_LOGI(TAG, "No SD card attached");
    return false;
  }

  ESP_LOGI(TAG, "SD Card Type: %s", cardType == CARD_MMC ? "MMC" : cardType == CARD_SD ? "SDSC"
                                                               : cardType == CARD_SDHC ? "SDHC"
                                                                                       : "UNKNOWN");

  return true; // sd initialization check passes
}
bool sdCardInitialized;

void saveTask(void *pvParameters)
{
  AITaskParams_t *params = (AITaskParams_t *)pvParameters;
  QueueHandle_t jpegQueue = params->jpegQueue;
  JpegImage image;
  File dump_file;

  while (true)
  {

    if (!sdCardInitialized)
    {
      sdCardInitialized = setupSdCard();
      dump_file = SD.open("/dump_jpeg.mjpeg", FILE_WRITE);
      if (sdCardInitialized)
      {
        ESP_LOGI(TAG, "SD Card initialized");
      }
      else
      {
        ESP_LOGE(TAG, "SD Card initialization failed");
        delay(1000);
      }
    }
    else
    {
      if (xQueueReceive(jpegQueue, &image, 0) == pdTRUE)
      {
        char filename[64];
        concatFileDump(dump_file, image.data, image.length);
        free(image.data);
        delay(10);
      }
      else
      {
        delay(10);
      }
    }
  }
}
/// @brief Saves jpeg images to sd
/// @param jpegQueue freeRTOS queue if images to save
void startSaveTask(QueueHandle_t jpegQueue)
{
  ESP_LOGI("AI_TASK", "Starting sd save task");
  AITaskParams_t *params = (AITaskParams_t *)malloc(sizeof(AITaskParams_t));
  params->jpegQueue = jpegQueue;

  xTaskCreate(
      saveTask,    // Task function
      "Save Task", // Name of the task (for debugging purposes)
      8000,        // Stack size (bytes)
      params,      // Parameter to pass to the task
      1,           // Task priority
      NULL         // Task handle
  );
}