#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "wifikeys.h"
#include <FreeRTOS.h>
#include "MjpegStreamState.h"
#include "JpegImage.h"
#include "Message.h"
#include "MjpegHandlers.h"
#include "websocket_task.h"
#include "capture_task.h"
#include "ai_task.h"
#include "save_task.h"
QueueHandle_t mjpegQueue;
QueueHandle_t saveJpegQueue;
QueueHandle_t AIjpegQueue;
QueueHandle_t messageQueue;
QueueHandle_t jpegQueues[3];
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// put function declarations here:
void notFound(AsyncWebServerRequest *request);
void setupWifi();
void setupServer();
void configureLogs()
{
  esp_log_level_set("MJPEG STREAM", ESP_LOG_WARN);
}
void setup()
{
  // delay for 10 seconds to allow for serial monitor to connect
  // delay(15000);
  configureLogs();
  Serial.begin(115200);
  delay(10000);
  ESP_LOGI("SETUP", "Starting setup");
  mjpegQueue = jpegQueues[0] = xQueueCreate(1, sizeof(JpegImage));
  AIjpegQueue = jpegQueues[1] = xQueueCreate(1, sizeof(JpegImage));
  saveJpegQueue = jpegQueues[2] = xQueueCreate(1, sizeof(JpegImage));

  messageQueue = xQueueCreate(1, sizeof(Message));
  startCaptureTask(jpegQueues, 3);
  startSaveTask(saveJpegQueue);

  setupWifi();
  setupServer();

  startAITask(AIjpegQueue, messageQueue);
  startWebsocket(messageQueue, &ws, &server);
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(50);
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
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void setupServer()
{
  server.on("/mjpeg", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleMjpeg(request, mjpegQueue); });

  server.onNotFound(notFound);
  server.begin();
}