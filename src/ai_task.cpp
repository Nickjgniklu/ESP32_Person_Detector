#include "ai_task.h"
#include "JpegImage.h"
#include <Arduino.h>

typedef struct
{
  QueueHandle_t jpegQueue;
} AITaskParams_t;

void aiTask(void *pvParameters)
{
  AITaskParams_t *params = (AITaskParams_t *)pvParameters;
  QueueHandle_t jpegQueue = params->jpegQueue;
  JpegImage image;
  ESP_LOGI("AI_TASK", "Starting AI Task");
  while (true)
  {

    if (xQueueReceive(jpegQueue, &image, 0) == pdTRUE)
    {
      ESP_LOGI("AI_TASK", "Received image of size %d", image.length);
      delay(1000);
      free(image.data);
    }
    else
    {
      ESP_LOGI("AI_TASK", "No image received");
      delay(100);
    }
  }
}
/// @brief Start inference task to process JPEG images
/// @param jpegQueue freeRTOS queue if images to process
void startAITask(QueueHandle_t jpegQueue)
{
  AITaskParams_t *params = (AITaskParams_t *)malloc(sizeof(AITaskParams_t));
  params->jpegQueue = jpegQueue;
  xTaskCreate(
      aiTask,    // Task function
      "AI Task", // Name of the task (for debugging purposes)
      2048,      // Stack size (bytes)
      params,    // Parameter to pass to the task
      1,         // Task priority
      NULL       // Task handle
  );
}