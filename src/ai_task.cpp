#include "ai_task.h"
#include "JpegImage.h"
#include <Arduino.h>
#include <MobileNetV2.h>
#include <all_ops_resolver.h>
#include "esp_camera.h"
#include <MobileNetClasses.h>
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Message.h>
typedef struct
{
  QueueHandle_t jpegQueue;
  QueueHandle_t messageQueue;
} AITaskParams_t;

uint raw_image_size = 240 * 240 * 3;

/// @brief holds rgb image data
uint8_t *rawBuffer;
namespace
{
  tflite::ErrorReporter *error_reporter = nullptr;
  const tflite::Model *model = nullptr;
  tflite::MicroInterpreter *interpreter = nullptr;
  TfLiteTensor *input = nullptr;
  // An area of memory to use for input, output, and intermediate arrays.
  const int kTensorArenaSize = 1024 * 1024;
  uint8_t *tensor_arena;
}

void initTFInterpreter()
{
  tensor_arena = (uint8_t *)ps_malloc(kTensorArenaSize);
  // TODO assert with unity that this init worked well
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  // Create Model
  model = tflite::GetModel(__MobileNetV2_tflite);
  // Verify Version of Tf Micro matches Model's verson
  if (model->version() != TFLITE_SCHEMA_VERSION)
  {
    error_reporter->Report(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return;
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
    error_reporter->Report("AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);
  error_reporter->Report("Input Shape");
  for (int i = 0; i < input->dims->size; i++)
  {
    error_reporter->Report("%d", input->dims->data[i]);
  }

  error_reporter->Report(TfLiteTypeGetName(input->type));
  error_reporter->Report("Output Shape");

  TfLiteTensor *output = interpreter->output(0);
  for (int i = 0; i < output->dims->size; i++)
  {
    error_reporter->Report("%d", output->dims->data[i]);
  }
  error_reporter->Report(TfLiteTypeGetName(output->type));
  error_reporter->Report("Arena Used:%d bytes of memory", interpreter->arena_used_bytes());
}

void aiTask(void *pvParameters)
{
  AITaskParams_t *params = (AITaskParams_t *)pvParameters;
  QueueHandle_t jpegQueue = params->jpegQueue;
  QueueHandle_t messageQueue = params->messageQueue;
  JpegImage image;
  ESP_LOGI("AI_TASK", "Starting AI Task");
  while (true)
  {

    if (xQueueReceive(jpegQueue, &image, 0) == pdTRUE)
    {
      ESP_LOGI("AI_TASK", "Received image of size %d", image.length);
      ESP_LOGI("AI_TASK", "Image decompress started");
      fmt2rgb888(image.data, image.length, PIXFORMAT_JPEG, rawBuffer);
      ESP_LOGI("AI_TASK", "Image decompressed");
      free(image.data);
      memcpy(input->data.uint8, rawBuffer, raw_image_size);

      ESP_LOGI("AI_TASK", "Model invoking started");
      uint start = millis();
      if (kTfLiteOk != interpreter->Invoke())
      {
        ESP_LOGE("AI_TASK", "Invoke failed.");
      }
      else
      {
        ESP_LOGI("AI_TASK", "Invoke passed. Took : %d milliseconds", millis() - start);
      }

      for (int i = 0; i < interpreter->outputs_size(); i++)
      {
        TfLiteTensor *output = interpreter->output(i);
        for (int j = 0; j < output->dims->size; j++)
        {
          ESP_LOGI("AI_TASK", "Output Shape %d", output->dims->data[j]);
        }
      }
      std::vector<std::pair<int8_t, int>> value_index_pairs;

      TfLiteTensor *output = interpreter->output(0);
      for (int i = 0; i < 1000; ++i)
      {
        value_index_pairs.push_back({output->data.int8[i], i});
      }
      std::sort(value_index_pairs.begin(), value_index_pairs.end(), [](const std::pair<int8_t, int> &a, const std::pair<int8_t, int> &b)
                { return a.first > b.first; });

      for (int i = 0; i < 5 && i < value_index_pairs.size(); ++i)
      {
        ESP_LOGI("AI_TASK", "Top %d: %s (%d)", i, classes[value_index_pairs[i].second].c_str(), value_index_pairs[i].first);
      }
      Message message;
      message.data = (char *)malloc(100);
      message.length = sprintf(message.data, "Top %d: %s (%d):%d", 1, classes[value_index_pairs[0].second].c_str(), value_index_pairs[0].first, value_index_pairs[0].second);
      xQueueSend(messageQueue, &message, 0);
    }
    else
    {
      ESP_LOGI("AI_TASK", "No image received");
      delay(10);
    }
  }
}
/// @brief Start inference task to process JPEG images
/// @param jpegQueue freeRTOS queue if images to process
void startAITask(QueueHandle_t jpegQueue, QueueHandle_t messageQueue)
{
  ESP_LOGI("AI_TASK", "Starting AI Task");
  ESP_LOGI("AI_TASK", "Creating raw image buffer of size %d", raw_image_size);
  rawBuffer = (uint8_t *)ps_malloc(raw_image_size);
  ESP_LOGI("AI_TASK", "Created raw image buffer of size %d", raw_image_size);
  ESP_LOGI("AI_TASK", "Creating tensorflow interpreter");
  initTFInterpreter();
  ESP_LOGI("AI_TASK", "Created tensorflow interpreter");
  AITaskParams_t *params = (AITaskParams_t *)malloc(sizeof(AITaskParams_t));
  params->jpegQueue = jpegQueue;
  params->messageQueue = messageQueue;
  xTaskCreate(
      aiTask,    // Task function
      "AI Task", // Name of the task (for debugging purposes)
      8000,      // Stack size (bytes)
      params,    // Parameter to pass to the task
      1,         // Task priority
      NULL       // Task handle
  );
}