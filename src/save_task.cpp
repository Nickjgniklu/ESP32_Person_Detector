#include "save_task.h"
#include "JpegImage.h"
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#define TAG "SAVE_TASK"
#define SD_CARD_PIN 21
static u64_t file_count = 0;

typedef struct
{
  QueueHandle_t jpegQueue;
} AITaskParams_t;

// SD card write file
void writeFile(fs::FS &fs, const char *path, uint8_t *data, size_t len)
{
  ESP_LOGI(TAG, "Writing file: %s", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    ESP_LOGI(TAG, "Failed to open file for writing");
    return;
  }
  if (file.write(data, len) == len)
  {
    ESP_LOGI(TAG, "File written");
  }
  else
  {
    ESP_LOGI(TAG, "Write failed");
  }
  file.close();
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
  if (!SD.begin(SD_CARD_PIN))
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
  while (true)
  {
    if (!sdCardInitialized)
    {
      sdCardInitialized = setupSdCard();
      if (sdCardInitialized)
      {
        ESP_LOGI(TAG, "SD Card initialized");
        file_count = countFilesInRoot(SD);
        ESP_LOGI(TAG, "File count: %d", file_count);
      }
      else
      {
        ESP_LOGE(TAG, "SD Card initialization failed");
      }
    }
    else
    {
      if (xQueueReceive(jpegQueue, &image, 0) == pdTRUE)
      {
        if (sdCardInitialized)
        {
          char filename[32];
          sprintf(filename, "/image%d.jpg", file_count++);
          ESP_LOGI(TAG, "Saving image to %s", filename);

          writeFile(SD, filename, image.data, image.length);
          free(image.data);
        }
      }
      else
      {
        if (sdCardInitialized)
        {
          delay(10);
        }
        else
        {
          delay(1000);
        }
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