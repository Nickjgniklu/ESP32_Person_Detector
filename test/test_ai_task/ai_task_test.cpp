#include "Arduino.h"
#include <FS.h>
#include <FreeRTOS.h>
#include <WiFiClient.h>
#include "ai_task.h"
#include "JpegImage.h"
#include "dude_jpeg.h"
#include "Message.h"

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
void test_send_result(void)
{
    QueueHandle_t jpegQueue = xQueueCreate(1, sizeof(JpegImage));
    QueueHandle_t messageQueue = xQueueCreate(1, sizeof(Message));
    startAITask(jpegQueue, messageQueue);

    JpegImage img;
    img.length = __dude_jpg_len;
    img.data = (u8_t *)malloc(img.length);
    memcpy(img.data, __dude_jpg, img.length);

    // queue result
    BaseType_t sendResult = xQueueSend(jpegQueue, &img, 0);
    TEST_ASSERT_EQUAL(pdTRUE, sendResult);
    Message message;
    BaseType_t receiveResult = xQueueReceive(messageQueue, &message, pdMS_TO_TICKS(10000));
    TEST_ASSERT_EQUAL(pdTRUE, receiveResult);

    TEST_ASSERT_EQUAL_STRING("{\"responseType\":\"prediction\",\"topPredictionStrength\":99,\"topPredictionIndex\":223,\"topPredictionClassName\":\"schipperke\"}", message.data);
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_send_result);
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
