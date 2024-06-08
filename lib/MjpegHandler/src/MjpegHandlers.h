#pragma once

#include <FreeRTOS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <JpegImage.h>
#include "MjpegStreamState.h"
void handleMjpeg(AsyncWebServerRequest *request, QueueHandle_t mjpegQueue);
size_t handleChunkedResponse(MjpegStreamState *state, QueueHandle_t mjpegQueue, uint8_t *buffer, size_t maxLen, size_t index);
