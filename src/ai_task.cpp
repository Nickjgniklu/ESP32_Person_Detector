#include "ai_task.h"
#include "JpegImage.h"
#include <Arduino.h>
#include <MobileNetV2.h>
#include <all_ops_resolver.h>
#include "esp_camera.h"

#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
typedef struct
{
  QueueHandle_t jpegQueue;
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

        ESP_LOGI("AI_TASK", "Output Shape %d", output->dims->size);
      }
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
void startAITask(QueueHandle_t jpegQueue)
{
  rawBuffer = (uint8_t *)ps_malloc(raw_image_size);

  initTFInterpreter();
  AITaskParams_t *params = (AITaskParams_t *)malloc(sizeof(AITaskParams_t));
  params->jpegQueue = jpegQueue;
  xTaskCreate(
      aiTask,    // Task function
      "AI Task", // Name of the task (for debugging purposes)
      8000,      // Stack size (bytes)
      params,    // Parameter to pass to the task
      1,         // Task priority
      NULL       // Task handle
  );
}