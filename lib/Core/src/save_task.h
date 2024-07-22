#pragma once
#include <FreeRTOS.h>
#include <queue.h> // Include the header file that defines QueueHandle_t

void startSaveTask(QueueHandle_t jpegQueue);