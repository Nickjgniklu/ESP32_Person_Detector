#include "ai_task.h"
#include "JpegImage.h"
#include <Arduino.h>
#include <Model.h>
#include <all_ops_resolver.h>
#include "esp_camera.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Message.h>
#include "esp_task_wdt.h"
#include "messages.h"
#define LOCAL_TAG "AI_TASK"
typedef struct
{
  QueueHandle_t jpegQueue;
  QueueHandle_t messageQueue;
} AITaskParams_t;

uint raw_image_size = 800 * 600 * 3;

/// @brief holds rgb image data
namespace
{
  tflite::ErrorReporter *error_reporter = nullptr;
  const tflite::Model *model = nullptr;
  tflite::MicroInterpreter *interpreter = nullptr;
  TfLiteTensor *input = nullptr;
  // An area of memory to use for input, output, and intermediate arrays.
  const int kTensorArenaSize = 6024 * 1024;
  uint8_t *tensor_arena;
  Model modelLoader = Model();
}

TfLiteStatus initTFInterpreter()
{
  tensor_arena = (uint8_t *)ps_malloc(kTensorArenaSize);
  // TODO assert with unity that this init worked well
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  // Create Model
  modelLoader.load_model("/model.tflite");
  model = tflite::GetModel(modelLoader.get_model());
  // Verify Version of Tf Micro matches Model's verson
  if (model->version() != TFLITE_SCHEMA_VERSION)
  {
    ESP_LOGE(LOCAL_TAG,
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return kTfLiteError;
  }
  CREATE_ALL_OPS_RESOLVER(op_resolver)
  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, op_resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk)
  {
    ESP_LOGE(LOCAL_TAG,"AllocateTensors() failed");
    return kTfLiteError;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);
  ESP_LOGI(LOCAL_TAG,"Input Shape");
  for (int i = 0; i < input->dims->size; i++)
  {
    ESP_LOGI(LOCAL_TAG,"%d", input->dims->data[i]);
  }

  ESP_LOGI(LOCAL_TAG,"%s",TfLiteTypeGetName(input->type));
  ESP_LOGI(LOCAL_TAG,"Output Shape");

  TfLiteTensor *output = interpreter->output(0);
  for (int i = 0; i < output->dims->size; i++)
  {
    ESP_LOGI(LOCAL_TAG,"%d", output->dims->data[i]);
  }
  ESP_LOGI(LOCAL_TAG,"%s",TfLiteTypeGetName(output->type));
  ESP_LOGI(LOCAL_TAG,"Arena Used:%d bytes of memory", interpreter->arena_used_bytes());
  return kTfLiteOk;
}
void sendMessageToQueue(const String &pmessage, QueueHandle_t messageQueue)
{
  size_t jsonLength = pmessage.length() + 1;
  Message message;
  message.data = (char *)malloc(jsonLength);
  if (message.data == nullptr)
  {
    // Handle memory allocation failure
    ESP_LOGE(LOCAL_TAG, "Failed to allocate memory for message");
    return;
  }
  memccpy(message.data, pmessage.c_str(), 0, jsonLength);
  message.length = jsonLength;
  xQueueSend(messageQueue, &message, 0);
}
void aiTask(void *pvParameters)
{
  AITaskParams_t *params = (AITaskParams_t *)pvParameters;
  QueueHandle_t jpegQueue = params->jpegQueue;
  QueueHandle_t messageQueue = params->messageQueue;
  JpegImage image;
  ESP_LOGI(LOCAL_TAG, "Starting AI Task");
  while (true)
  {

    if (xQueueReceive(jpegQueue, &image, 0) == pdTRUE)
    {
      ESP_LOGI(LOCAL_TAG, "Received image of size %d", image.length);
      ESP_LOGI(LOCAL_TAG, "Image decompress started");
      fmt2rgb888(image.data, image.length, PIXFORMAT_JPEG, input->data.uint8);
      ESP_LOGI(LOCAL_TAG, "Image decompressed");
      free(image.data);
      delay(10); // feed watchdog
      ESP_LOGI(LOCAL_TAG, "Model invoking started");
      uint start = millis();
      esp_task_wdt_delete(xTaskGetCurrentTaskHandle());
      if (kTfLiteOk != interpreter->Invoke())
      {
        ESP_LOGE(LOCAL_TAG, "Invoke failed.");
      }
      else
      {
        ESP_LOGI(LOCAL_TAG, "Invoke passed. Took : %d milliseconds", millis() - start);
      }
      esp_task_wdt_add(xTaskGetCurrentTaskHandle());

      for (int i = 0; i < interpreter->outputs_size(); i++)
      {
        TfLiteTensor *output = interpreter->output(i);
        for (int j = 0; j < output->dims->size; j++)
        {
          ESP_LOGI(LOCAL_TAG, "Output Shape %d", output->dims->data[j]);
        }
      }
      std::vector<std::pair<int8_t, int>> value_index_pairs;

      TfLiteTensor *output = interpreter->output(0);
      int result = output->data.int8[0];
      float result_float = (result - modelLoader.get_quantization_zero_point()) * modelLoader.get_quantization_scale();
      ESP_LOGI(LOCAL_TAG,"Float result: %f", result_float);
      bool person = result_float > 0.15; // default_model_prediction_threshold;
      if (person)
      {
        ESP_LOGI(LOCAL_TAG, "Person detected");
      }
      else
      {
        ESP_LOGI(LOCAL_TAG, "No person detected");
      }
      String message = person ? "person" : "no_person";

      String pmessage = predictionMessage(message, result_float);
      sendMessageToQueue(pmessage, messageQueue);
    }
    else
    {
      delay(10);
    }
  }
}
/// @brief Start inference task to process JPEG images
/// @param jpegQueue freeRTOS queue if images to process
void startAITask(QueueHandle_t jpegQueue, QueueHandle_t messageQueue)
{
  ESP_LOGI(LOCAL_TAG, "Starting AI Task");
  ESP_LOGI(LOCAL_TAG, "Creating tensorflow interpreter");
  TfLiteStatus initStatus = initTFInterpreter();

  if (initStatus != kTfLiteOk)
  {
    ESP_LOGE(LOCAL_TAG, "Failed to initialize tensorflow interpreter, ai task will not start");
  }else{
    

  ESP_LOGI(LOCAL_TAG, "Created tensorflow interpreter");
  AITaskParams_t *params = (AITaskParams_t *)malloc(sizeof(AITaskParams_t));
  params->jpegQueue = jpegQueue;
  params->messageQueue = messageQueue;
  xTaskCreatePinnedToCore(
      aiTask,    // Task function
      "AI Task", // Name of the task (for debugging purposes)
      8000,      // Stack size (bytes)
      params,    // Parameter to pass to the task
      1,         // Task priority
      NULL,      // Task handle
      1          // Run the on other core from wifi stuuff
  );
  ESP_LOGI(LOCAL_TAG, "Initialized tensorflow interpreter");
  }
}