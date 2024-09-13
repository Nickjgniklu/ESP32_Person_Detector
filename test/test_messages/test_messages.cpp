#include "Arduino.h"
#include <FreeRTOS.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "Model.h"
#include "JpegImage.h"
#include "MjpegHandlers.h"
#include "messages.h"

#include "unity.h"
// TODO test message creation for ws
void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}

void test_system_message(void)
{
    String message = systemInfoMessage();
    JsonDocument json;
    DeserializationError error = deserializeJson(json, message);
    TEST_ASSERT_EQUAL(error.Ok, ArduinoJson::DeserializationError::Ok);
    TEST_ASSERT_TRUE(json.containsKey("responseType"));
    TEST_ASSERT_TRUE(json.containsKey("heapSize"));
    TEST_ASSERT_TRUE(json.containsKey("sdkVersion"));
    TEST_ASSERT_TRUE(json.containsKey("chipModel"));
    TEST_ASSERT_TRUE(json.containsKey("freeHeap"));
    TEST_ASSERT_TRUE(json.containsKey("freePsram"));
    TEST_ASSERT_TRUE(json.containsKey("psRamSize"));
    TEST_ASSERT_TRUE(json.containsKey("uptimeMs"));
}
void test_sd_message(void)
{
    String message = sdInfoMessage();
    JsonDocument json;
    DeserializationError error = deserializeJson(json, message);
    TEST_ASSERT_EQUAL(error.Ok, ArduinoJson::DeserializationError::Ok);
    TEST_ASSERT_TRUE(json.containsKey("responseType"));
    TEST_ASSERT_TRUE(json.containsKey("totalBytes"));
    TEST_ASSERT_TRUE(json.containsKey("usedBytes"));
    TEST_ASSERT_TRUE(json.containsKey("freeBytes"));
}

void test_prediction_message(void)
{
    String className = "cat";
    float prob = 0.2;
    uint32_t index = 1;
    String message = predictionMessage(className, prob);
    JsonDocument json;
    DeserializationError error = deserializeJson(json, message);
    TEST_ASSERT_EQUAL(error.Ok, ArduinoJson::DeserializationError::Ok);

    // Validate the existence of each key
    TEST_ASSERT_TRUE(json.containsKey("responseType"));
    TEST_ASSERT_TRUE(json.containsKey("probability"));
    TEST_ASSERT_TRUE(json.containsKey("prediction"));

    // Validate the values
    TEST_ASSERT_EQUAL_STRING("prediction", json["responseType"]);
    TEST_ASSERT_EQUAL(prob, json["probability"]);
    // TEST_ASSERT_EQUAL(className, json["prediction"]);
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_system_message);
    RUN_TEST(test_sd_message);
    RUN_TEST(test_prediction_message);
    return UNITY_END();
}

void setup()
{
    // Wait ~2 seconds before the Unity test runner
    // establishes connection with a board Serial interface
    delay(5000);

    runUnityTests();
}
void loop() {}
