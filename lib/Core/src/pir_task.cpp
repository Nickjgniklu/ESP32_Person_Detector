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
  QueueHandle_t pirEventQueue;
} TaskParams_t;



void monitorPir(void *pvParameters)
{
  TaskParams_t *params = (TaskParams_t *)pvParameters;
  QueueHandle_t jpegQueue = params->pirEventQueue;
  JpegImage image;

  while (true)
  {

    
      if (xQueueReceive(jpegQueue, &image, 0) == pdTRUE)
      {
        char filename[64];
        free(image.data);
        delay(10);
      }
      else
      {
        delay(10);
      }
    }
}
/// @brief Detect motions with pir sensor 
/// @param jpegQueue freeRTOS queue if images to save
void startPirTask(QueueHandle_t pirEventQueue)
{
  ESP_LOGI("PIR_TASK", "Starting pir monitoring task");
  TaskParams_t *params = (TaskParams_t *)malloc(sizeof(TaskParams_t));
  params->pirEventQueue = pirEventQueue;

  xTaskCreate(
    monitorPir,    // Task function
      "Monitor PIR", // Name of the task (for debugging purposes)
      8000,        // Stack size (bytes)
      params,      // Parameter to pass to the task
      1,           // Task priority
      NULL         // Task handle
  );
}