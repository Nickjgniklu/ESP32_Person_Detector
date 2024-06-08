#pragma once
#include <FreeRTOS.h>
#include <ESPAsyncWebServer.h>
#include <queue.h> // Include the header file that defines QueueHandle_t
void startWebsocket(QueueHandle_t messageQueue, AsyncWebSocket *ws, AsyncWebServer *server);