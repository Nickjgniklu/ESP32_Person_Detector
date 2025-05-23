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
#include "ota_task.h"
#include "SPIFFS.h"
#include <SD.h>

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
void listFiles(AsyncWebServerRequest *request);
void configureLogs()
{
  esp_log_level_set("*", ESP_LOG_ERROR);
  //esp_log_level_set("MJPEG_STREAM", ESP_LOG_INFO);
   //esp_log_level_set("OTA_TASK", ESP_LOG_INFO);
  // esp_log_level_set("CAMERA_TASK", ESP_LOG_WARN);
  // esp_log_level_set("MODAL", ESP_LOG_WARN);
  // esp_log_level_set("MESSAGES", ESP_LOG_WARN);
  esp_log_level_set("AI_TASK", ESP_LOG_INFO);
  // esp_log_level_set("FILE_HELPERS", ESP_LOG_WARN);
}
void setup()
{
  // delay for 10 seconds to allow for serial monitor to connect
  // delay(15000);
  Serial.begin(115200);
  configureLogs();

  delay(10000);
  ESP_LOGI("SETUP", "Starting setup");
  if (!SPIFFS.begin(true))
  {
    ESP_LOGE("SETUP", "SPIFFs failure");
  }
  mjpegQueue = jpegQueues[0] = xQueueCreate(2, sizeof(JpegImage));
  AIjpegQueue = jpegQueues[1] = xQueueCreate(1, sizeof(JpegImage));
  saveJpegQueue = jpegQueues[2] = xQueueCreate(2, sizeof(JpegImage));

  messageQueue = xQueueCreate(30, sizeof(Message));
  esp_log_set_vprintf([](const char *fmt, va_list args) -> int {

    //this function needs to be rentrant
    // if the message queue has space create a message in it
    int length = vprintf(fmt, args);
    const uint maxLength = 100;

    Message message;
    message.data = (char *)malloc(maxLength);
    if (message.data == nullptr)
    {
      return vprintf(fmt, args);
    }

    // For debugging, put "hello world" in the data field
    int web_log_length =vsnprintf(message.data, maxLength, fmt, args); // format the message into the data field
    message.length = web_log_length+ 1; // set the length of the message (including null terminator)

    // if the queue is full this will silently fail to send the message
    xQueueSend(messageQueue, &message, 0);


    return length;
  });
  startCaptureTask(jpegQueues, 3);
  startSaveTask(saveJpegQueue);

  setupWifi();
  setupServer();

  startAITask(AIjpegQueue, messageQueue);
  startWebsocket(messageQueue, &ws, &server);
  startOtaTask(); 
  
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

void sendUI(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html.gz", "text/html");
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void listFiles(AsyncWebServerRequest *request)
{
  if (!SD.begin())
  {
    request->send(500, "text/plain", "SD Card Mount Failed");
    return;
  }

  File root = SD.open("/");
  if (!root)
  {
    request->send(500, "text/plain", "Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    request->send(500, "text/plain", "Not a directory");
    return;
  }

  String output = "<html><body><h1>Files:</h1><ul>";
  File file = root.openNextFile();
  int fileCount = 0;
  while (file && fileCount < 10)
  {
    if (file.isDirectory())
    {
      output += "<li><b>DIR</b> ";
      output += file.name();
      output += "</li>";
    }
    else
    {
      output += "<li><a href=\"/download?filename=";
      output += file.name();
      output += "\">";
      output += file.name();
      output += "</a> (";
      output += file.size();
      output += " bytes)</li>";
    }
    file = root.openNextFile();
    fileCount++;
  }
  output += "</ul></body></html>";

  request->send(200, "text/html", output);
}

void downloadFile(AsyncWebServerRequest *request)
{
  if (!request->hasParam("filename"))
  {
    request->send(400, "text/plain", "Bad Request");
    return;
  }

  String filename = request->getParam("filename")->value();
  File file = SD.open(filename);
  if (!file)
  {
    request->send(404, "text/plain", "File Not Found");
    return;
  }

  AsyncWebServerResponse *response = request->beginResponse(SD, filename, "application/octet-stream");
  request->send(response);
}

void deleteFile(AsyncWebServerRequest *request)
{
  if (!request->hasParam("filename"))
  {
    request->send(400, "text/plain", "Bad Request");
    return;
  }

  String filename = request->getParam("filename")->value();
  if (!SD.exists(filename))
  {
    request->send(404, "text/plain", "File Not Found");
    return;
  }

  if (SD.remove(filename))
  {
    request->send(200, "text/plain", "File Deleted");
  }
  else
  {
    request->send(500, "text/plain", "Failed to Delete File");
  }
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
  server.on("/", sendUI);
  server.on("/listFiles", HTTP_GET, listFiles);
  server.on("/download", HTTP_GET, downloadFile);
  server.on("/delete", HTTP_GET, deleteFile); // Add this line

  server.onNotFound(notFound);
  server.begin();
}