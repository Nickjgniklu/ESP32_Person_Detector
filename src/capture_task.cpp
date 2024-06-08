#include "capture_task.h"
#include "camera_config.h"
#include "JpegImage.h"
#include "OV2640.h" // Include the header file that defines the "OV2640" type
#define TAG "CAMERA_TASK"
OV2640 camera;

typedef struct
{
  QueueHandle_t *jpegQueues;
  uint jpegQueueCount;
} CameraTaskParams_t;
void setupCamera()
{
  ESP_LOGI("CAMERA", "Starting Camera");
  esp_err_t err = camera.init(esp32cam_ESPCam_config);
  if (ESP_OK != err)
  {
    ESP_LOGE("CAMERA", "Failed to start camera, will restart in 10s, error: %s", esp_err_to_name(err));
    delay(10000);

    // ESP.restart();
  }
  ESP_LOGI("CAMERA", "Started Camera");
}

void cameraTask(void *pvParameters)
{
  CameraTaskParams_t *params = (CameraTaskParams_t *)pvParameters;
  QueueHandle_t *jpegQueues = params->jpegQueues;
  uint jpegQueueCount = params->jpegQueueCount;

  while (true)
  {
    // check if any of the queues have space
    bool anyQueueHasSpace = false;
    for (uint i = 0; i < jpegQueueCount; i++)
    {
      if (uxQueueSpacesAvailable(jpegQueues[i]) > 0)
      {
        anyQueueHasSpace = true;
        break;
      }
    }
    if (anyQueueHasSpace)
    {
      camera.run();

      for (uint i = 0; i < jpegQueueCount; i++)
      {
        size_t frame_size = camera.getSize();
        uint8_t *frame_buffer = (uint8_t *)malloc(frame_size);
        memcpy(frame_buffer, camera.getfb(), frame_size);
        JpegImage image;

        if (!getLocalTime(&image.timeInfo))
        {
          ESP_LOGE(TAG, "Failed to obtain time");
        }
        image.data = frame_buffer;
        image.length = frame_size;
        if (uxQueueSpacesAvailable(jpegQueues[i]) > 0)
        {
          if (xQueueSend(jpegQueues[i], &image, 0) != pdPASS)
          {
            ESP_LOGE("CAMERA_TASK", "Failed to send frame to queue");
            free(frame_buffer);
          }
          else
          {
            ESP_LOGI("CAMERA_TASK", "Sent frame to queue %d", i);
          }
        }
        else
        {
          ESP_LOGI("CAMERA_TASK", "Queue %d is full", i);
          free(frame_buffer);
        }
      }
    }
    else
    {
      // dont check too often
      delay(10);
    }
  }
}
/// @brief Start a camera task to fill a queues with JPEG images
/// @param jpegQueues freeRTOS queues to fill with JPEG images
/// @param jpegQueueCount number of queues
void startCaptureTask(QueueHandle_t *jpegQueues, uint jpegQueueCount)
{
  setupCamera();
  CameraTaskParams_t *params = (CameraTaskParams_t *)malloc(sizeof(CameraTaskParams_t));
  // Dynamically allocate memory for the queues
  params->jpegQueues = jpegQueues;
  params->jpegQueueCount = jpegQueueCount;
  xTaskCreate(
      cameraTask,    // Task function
      "Camera Task", // Name of the task (for debugging purposes)
      8000,          // Stack size (bytes)
      params,        // Parameter to pass to the task
      1,             // Task priority
      NULL           // Task handle
  );
}