#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "wifikeys.h"
#include <FreeRTOS.h>
#include "MjpegStreamState.h"
#include "JpegImage.h"
#include "MjpegHandlers.h"
#include "capture_task.h"
QueueHandle_t mjpegQueue;

AsyncWebServer server(80);

// put function declarations here:
void notFound(AsyncWebServerRequest *request);
void setupWifi();
void setupServer();

void setup()
{
  // delay for 10 seconds to allow for serial monitor to connect
  delay(15000);
  ESP_LOGI("SETUP", "Starting setup");
  mjpegQueue = xQueueCreate(4, sizeof(JpegImage));

  Serial.begin(115200);
  startCaptureTask(mjpegQueue);
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