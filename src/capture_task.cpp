#include "capture_task.h"
#include "camera_config.h"
#include "JpegImage.h"
#include "OV2640.h" // Include the header file that defines the "OV2640" type

OV2640 camera;
typedef struct
{
  QueueHandle_t jpegQueue;
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
  QueueHandle_t jpegQueue = params->jpegQueue;
  while (true)
  {
    if (uxQueueSpacesAvailable(jpegQueue) > 0)
    {
      camera.run();

      size_t frame_size = camera.getSize();
      uint8_t *frame_buffer = (uint8_t *)ps_malloc(frame_size);
      memcpy(frame_buffer, camera.getfb(), frame_size);
      JpegImage image;
      image.data = frame_buffer;
      image.length = frame_size;
      if (xQueueSend(jpegQueue, &image, 0) != pdPASS)
      {
        ESP_LOGE("CAMERA_TASK", "Failed to send frame to queue");
        free(frame_buffer);
      }
      else
      {
        ESP_LOGI("CAMERA_TASK", "Sent frame to queue");
      }
    }
    else
    {
      // dont check too often
      delay(10);
    }
  }
}
/// @brief Start a camera task to fill a queue with JPEG images
/// @param jpegQueue freeRTOS queue to fill with JPEG images
void startCaptureTask(QueueHandle_t jpegQueue)
{
  setupCamera();
  CameraTaskParams_t *params = (CameraTaskParams_t *)malloc(sizeof(CameraTaskParams_t));
  params->jpegQueue = jpegQueue;
  xTaskCreate(
      cameraTask,    // Task function
      "Camera Task", // Name of the task (for debugging purposes)
      2048,          // Stack size (bytes)
      params,        // Parameter to pass to the task
      1,             // Task priority
      NULL           // Task handle
  );
}