#include "Arduino.h"
#include <FreeRTOS.h>
#include <WiFiClient.h>
#include "MobileNetV2.h"
#include "save_task.h"
#include "JpegImage.h"
#include "Message.h"
#include "dude_jpeg.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
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
void test_save_image(void)
{
    QueueHandle_t jpegQueue = xQueueCreate(1, sizeof(JpegImage));
    startSaveTask(jpegQueue);
    JpegImage img;
    img.length = __dude_jpg_len;
    img.data = (u8_t *)malloc(img.length);

    tm timeInfo = {};
    timeInfo.tm_year = 2023 - 1900; // Year since 1900
    timeInfo.tm_mon = 7 - 1;        // Month (0-11, so 9-1 for September)
    timeInfo.tm_mday = 25;          // Day of the month
    timeInfo.tm_hour = 14;          // Hours since midnight (0-23)
    timeInfo.tm_min = 30;           // Minutes after the hour (0-59)
    timeInfo.tm_sec = 45;           // Seconds after the minute (0-60)

    // Format the time into a string
    char expectedFileName[100];
    strftime(expectedFileName, sizeof(expectedFileName), "/%B %d %Y %H %M %S.jpg", &timeInfo);

    img.timeInfo = timeInfo;
    memcpy(img.data, __dude_jpg, img.length);

    // queue result
    BaseType_t sendResult = xQueueSend(jpegQueue, &img, 0);
    TEST_ASSERT_EQUAL(pdTRUE, sendResult);
    // wait for upto 20 seconds for the file to be created

    delay(10'000);
    bool file_exists = SD.exists(expectedFileName);

    TEST_ASSERT_EQUAL(true, file_exists);
    File file = SD.open(expectedFileName, FILE_READ);

    // verify that a new file was created of the correct size
    TEST_ASSERT_EQUAL(img.length, file.size());
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_save_image);
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
