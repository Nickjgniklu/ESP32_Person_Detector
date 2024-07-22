#pragma once
#include <FreeRTOS.h>
#include <queue.h> // Include the header file that defines QueueHandle_t

void startAITask(QueueHandle_t jpegQueue, QueueHandle_t messageQueue);