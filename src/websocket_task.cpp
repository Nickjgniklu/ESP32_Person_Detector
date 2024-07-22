#include "websocket_task.h"
#include "Message.h"
#include <ArduinoJson.h>
#include "messages.h"
typedef struct
{
  QueueHandle_t messageQueue;
  AsyncWebSocket *ws;

} WebsocketTaskParams_t;
void sendSdInfo(AsyncWebSocketClient *client)
{
  client->text(sdInfoMessage());
}

void sendSystemInfo(AsyncWebSocketClient *client)
{
  client->text(systemInfoMessage());
}

void handleWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    ESP_LOGI("WEBSOCKET", "Received message: '%s'", data);
    JsonDocument json;
    DeserializationError error = deserializeJson(json, data);
    if (error)
    {
      ESP_LOGE("WEBSOCKET", "Received message not json: '%s'", data);
      return;
    }
    if (json.containsKey("requestType") && json["requestType"] == "sdInfo")
    {
      sendSdInfo(client);
    }
    if (json.containsKey("requestType") && json["requestType"] == "systemInfo")
    {
      sendSystemInfo(client);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:

    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(client, arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setupWebSocket(AsyncWebSocket *ws, AsyncWebServer *server)
{
  ESP_LOGI("WEBSOCKET", "Setting up websocket");
  ws->onEvent(onEvent);
  server->addHandler(ws);
  ESP_LOGI("WEBSOCKET", "Websocket setup done");
}

void startWebsocketTask(void *pvParameters)
{
  WebsocketTaskParams_t *params = (WebsocketTaskParams_t *)pvParameters;
  QueueHandle_t messageQueue = params->messageQueue;
  AsyncWebSocket *ws = params->ws;
  ESP_LOGI("WEBSOCKET", "Starting websocket task");

  while (true)
  {
    {
      Message message;
      if (xQueueReceive(messageQueue, &message, portMAX_DELAY))
      {
        ESP_LOGI("WEBSOCKET", "Sending message: %s", message.data);
        if (ws->count() != 0)
        {
          ws->textAll(message.data);
        }
        free(message.data);
      }
      else
      {
        // dont check too often
        delay(10);
      }
    }
  }
}

void startWebsocket(QueueHandle_t messageQueue, AsyncWebSocket *ws, AsyncWebServer *server)
{
  setupWebSocket(ws, server);
  WebsocketTaskParams_t *params = (WebsocketTaskParams_t *)malloc(sizeof(WebsocketTaskParams_t));
  // Dynamically allocate memory for the queues
  params->messageQueue = messageQueue;
  params->ws = ws;
  xTaskCreate(
      startWebsocketTask, // Task function
      "Camera Task",      // Name of the task (for debugging purposes)
      2048,               // Stack size (bytes)
      params,             // Parameter to pass to the task
      1,                  // Task priority
      NULL                // Task handle
  );
}