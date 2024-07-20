#include "Arduino.h"
#include <FreeRTOS.h>
#include <WiFiClient.h>

#include "JpegImage.h"
#include "MjpegHandlers.h"

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

int runUnityTests(void)
{
    UNITY_BEGIN();
    // RUN_TEST(test_mjpeg_chunks);
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
