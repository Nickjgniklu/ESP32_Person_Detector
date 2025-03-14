#include "save_task.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOCAL_TAG "OTA_TASK"

TaskHandle_t otaTaskHandle = NULL; // Global task handle

void setupOTA()
{
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("esp32doorbell");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with its md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    ESP_LOGI(LOCAL_TAG, "Start updating %s", type.c_str());
  });

  ArduinoOTA.onEnd([]() {
    ESP_LOGI(LOCAL_TAG, "\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    ESP_LOGI(LOCAL_TAG, "Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    ESP_LOGE(LOCAL_TAG, "Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      ESP_LOGE(LOCAL_TAG, "Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      ESP_LOGE(LOCAL_TAG, "Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      ESP_LOGE(LOCAL_TAG, "Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      ESP_LOGE(LOCAL_TAG, "Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      ESP_LOGE(LOCAL_TAG, "End Failed");
    }
  });

  ArduinoOTA.begin();
}

void otaTask(void *pvParameters)
{
  setupOTA();
  while (true)
  {
    ArduinoOTA.handle();
    delay(100);
  }
}

void startOtaTask()
{
  if (otaTaskHandle == NULL)
  {
    ESP_LOGI(LOCAL_TAG, "Starting OTA task");

    xTaskCreate(
        otaTask,    // Task function
        "OTA Task", // Name of the task (for debugging purposes)
        8000,       // Stack size (bytes)
        NULL,       // Parameter to pass to the task
        1,          // Task priority
        &otaTaskHandle // Task handle
    );
  }
  else
  {
    ESP_LOGI(LOCAL_TAG, "OTA task is already running");
  }
}

void stopOtaTask()
{
  if (otaTaskHandle != NULL)
  {
    vTaskDelete(otaTaskHandle);
    otaTaskHandle = NULL;
    ESP_LOGI(LOCAL_TAG, "OTA task stopped");
  }
  else
  {
    ESP_LOGI(LOCAL_TAG, "OTA task is not running");
  }
}
