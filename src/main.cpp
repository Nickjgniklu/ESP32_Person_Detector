#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "wifikeys.h"
#include <FreeRTOS.h>
#include "camera_config.h"
#include "MjpegStreamState.h"
#include "JpegImage.h"
#include "MjpegHandlers.h"

QueueHandle_t mjpegQueue;

AsyncWebServer server(80);
OV2640 camera;

// put function declarations here:
void notFound(AsyncWebServerRequest *request);
void setupWifi();
void setupServer();
void setupCamera();
void cameraTask(void *pvParameters);

void setup()
{
  // delay for 10 seconds to allow for serial monitor to connect
  delay(15000);
  ESP_LOGI("SETUP", "Starting setuo");
  mjpegQueue = xQueueCreate(4, sizeof(JpegImage));

  Serial.begin(115200);
  setupCamera();
  xTaskCreate(
      cameraTask,    // Task function
      "Camera Task", // Name of the task (for debugging purposes)
      2048,          // Stack size (bytes)
      NULL,          // Parameter to pass to the task
      1,             // Task priority
      NULL           // Task handle
  );
  setupWifi();
  setupServer();
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(1000);
}

// put function definitions here:
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

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

void setupWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    ESP_LOGE("WIFI_SETUP", "WiFi Failed! will restart in 10 second.");

    delay(10000);
    // ESP.restart();
  }
  ESP_LOGI("WIFI_SETUP", "IP Address: %s", WiFi.localIP().toString().c_str());
  delay(5000);
}

void setupServer()
{
  server.on("/mjpeg", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleMjpeg(request, mjpegQueue); });

  server.onNotFound(notFound);
  server.begin();
}

void cameraTask(void *pvParameters)
{
  while (true)
  {
    if (uxQueueSpacesAvailable(mjpegQueue) > 0)
    {
      camera.run();

      size_t frame_size = camera.getSize();
      uint8_t *frame_buffer = (uint8_t *)ps_malloc(frame_size);
      memcpy(frame_buffer, camera.getfb(), frame_size);
      JpegImage image;
      image.data = frame_buffer;
      image.length = frame_size;
      if (xQueueSend(mjpegQueue, &image, 0) != pdPASS)
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

//
//
//