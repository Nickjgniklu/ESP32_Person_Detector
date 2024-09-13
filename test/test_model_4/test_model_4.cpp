
#include "Arduino.h"
#include <FS.h>
#include <WiFiClient.h>
#include <ConversionTools.h>
#include "esp_camera.h"
#include <Model.h>
#include <all_ops_resolver.h>
#include "image_4.h"

#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "unity.h"

unsigned char label = label_4;
unsigned char *image = image_4;
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
    model = tflite::GetModel(__model_tflite);
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
void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}

void test_model(void)
{
    initTFInterpreter();
    uint raw_image_size = 240 * 240 * 3;

    memcpy(input->data.uint8, image, raw_image_size);

    int start = millis();
    error_reporter->Report("Invoking.");

    if (kTfLiteOk != interpreter->Invoke()) // Any error i have in invoke tend to just crash the whole system so i dont usually see this message
    {
        error_reporter->Report("Invoke failed.");
    }
    else
    {
        error_reporter->Report("Invoke passed.");
        error_reporter->Report(" Took : %d milliseconds", millis() - start);
    }

    TfLiteTensor *output = interpreter->output(0);
    int result = output->data.int8[0];
    float result_float = (result - model_quantization_zero_point) * model_quantization_scale;
    error_reporter->Report("Float result: %f", result_float);
    bool person = result_float > default_model_prediction_threshold;
    bool truth = label == 1;
    error_reporter->Report("predicted %d truth %d", person, truth);

    TEST_ASSERT_EQUAL_UINT(truth, person);
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_model);
    return UNITY_END();
}

void setup()
{
    // Wait ~2 seconds before the Unity test runner
    // establishes connection with a board Serial interface
    delay(2000);

    runUnityTests();
}
void loop() {}

