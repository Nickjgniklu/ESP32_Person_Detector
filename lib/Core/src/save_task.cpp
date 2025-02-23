#include "save_task.h"
#include "JpegImage.h"
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define LOCAL_TAG "SAVE_TASK"
#define SD_CARD_PIN 21

typedef struct
{
  QueueHandle_t jpegQueue;
} AITaskParams_t;

// SD card write file
void writeFile(fs::FS &fs, const char *path, uint8_t *data, size_t len)
{
  ESP_LOGI(LOCAL_TAG, "Writing file: %s", path);
  unsigned long start = millis();
  unsigned long openStart = millis();
  File file = fs.open(path, FILE_WRITE);
  unsigned long openEnd = millis();
  if (!file)
  {
    ESP_LOGE(LOCAL_TAG, "Failed to open file for writing: %s", path);
    return;
  }
  ESP_LOGI(LOCAL_TAG, "Time to open file: %lu ms", openEnd - openStart);

  size_t chunkSize = 512;
  size_t written = 0;
  while (written < len)
  {
    unsigned long chunkStart = millis();
    size_t toWrite = (len - written) < chunkSize ? (len - written) : chunkSize;
    if (file.write(data + written, toWrite) != toWrite)
    {
      ESP_LOGE(LOCAL_TAG, "Write failed at chunk starting at %d", written);
      file.close();
      return;
    }
    unsigned long chunkEnd = millis();
    ESP_LOGI(LOCAL_TAG, "Time to write chunk: %lu ms", chunkEnd - chunkStart);
    written += toWrite;
  }

  unsigned long end = millis();
  ESP_LOGI(LOCAL_TAG, "File of size %d written in %lu ms", len, end - start);
  file.close();
}

bool concatFileDump(fs::File &file, uint8_t *data, size_t len)
{
  if (!file)
  {
    ESP_LOGE(LOCAL_TAG, "file not open");
    return false;
  }

  unsigned long start = millis();
  size_t written = file.write(data, len);
  file.flush();
  unsigned long end = millis();

  if (written != len)
  {
    ESP_LOGE(LOCAL_TAG, "Failed to append data to file, wrote %d bytes", written);
    ESP_LOGE(LOCAL_TAG, "File write error: %d", file.getWriteError());
    file.clearWriteError();
    auto name = file.name();
    auto path = "/";
    file.close();
    ESP_LOGI(LOCAL_TAG, "File error reopening:%s %s", path, name);
    file = SD.open(String("/") + name, FILE_APPEND);
    if(!file){
    return false;
  }
  return true;
  }
  else
  {
    ESP_LOGI(LOCAL_TAG, "Appended %d bytes to file in %lu ms", written, end - start);
  }
  ESP_LOGI(LOCAL_TAG, "File size: %d", file.size());
  // limit file size to 100MB
  if(file.size() > 100000000){
    return false;
  }
  return true;
}

u64_t nextFileNumber(fs::FS &fs)
{
  File root = fs.open("/");
  if (!root)
  {
    ESP_LOGI(LOCAL_TAG, "Failed to open directory");
    return 0;
  }
  if (!root.isDirectory())
  {
    ESP_LOGI(LOCAL_TAG, "Not a directory");
    return 0;
  }

  File file = root.openNextFile();
  u64_t max = 0;
  while (file)
  {
    if (!file.isDirectory())
    {
      String name = file.name();
      int num = 0;
      if (sscanf(name.c_str(), "dump_jpeg%d.mjpeg", &num) == 1)
      {
        if (num > max)
        {
          max = num+1;
        }
      }
    }
    file = root.openNextFile();
  }
  ESP_LOGI(LOCAL_TAG, "File max file: %d", max);
  return max;
}

bool setupSdCard()
{
  // Initialize SD card
  if (!SD.begin(SD_CARD_PIN, SPI, 40'000'000u))
  {
    ESP_LOGI(LOCAL_TAG, "Card Mount Failed");
    return false;
  }
  uint8_t cardType = SD.cardType();

  // Determine if the type of SD card is available
  if (cardType == CARD_NONE)
  {
    ESP_LOGI(LOCAL_TAG, "No SD card attached");
    return false;
  }

  ESP_LOGI(LOCAL_TAG, "SD Card Type: %s", cardType == CARD_MMC ? "MMC" : cardType == CARD_SD ? "SDSC"
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
  auto curentFilenumber = 0;
  auto base_file_name = "/dump_jpeg";

  while (true)
  {

    if (!sdCardInitialized)
    {
      sdCardInitialized = setupSdCard();
      curentFilenumber = nextFileNumber(SD);
      
      auto file_name = base_file_name + String(curentFilenumber++) + ".mjpeg";
      ESP_LOGI(LOCAL_TAG, "Opening new file: %s", file_name.c_str());

      dump_file = SD.open(file_name, FILE_WRITE);
      if (sdCardInitialized)
      {
        ESP_LOGI(LOCAL_TAG, "SD Card initialized");
      }
      else
      {
        ESP_LOGE(LOCAL_TAG, "SD Card initialization failed");
        delay(1000);
      }
    }
    else
    {
      if (xQueueReceive(jpegQueue, &image, 0) == pdTRUE)
      {
        char filename[64];
        if(concatFileDump(dump_file, image.data, image.length)){
          ESP_LOGI(LOCAL_TAG, "File appended");
        }else{
          dump_file.close();
          ESP_LOGI(LOCAL_TAG, "File closed");
          auto file_name = base_file_name + String(curentFilenumber++) + ".mjpeg";
          ESP_LOGI(LOCAL_TAG, "Opening new file: %s", file_name.c_str());
          dump_file = SD.open(file_name, FILE_WRITE);
        }
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