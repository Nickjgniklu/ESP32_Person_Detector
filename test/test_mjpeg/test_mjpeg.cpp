#include "Arduino.h"
#include <FreeRTOS.h>
#include <WiFiClient.h>

#include "JpegImage.h"
#include "MjpegHandlers.h"

#include "unity.h"

void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}

// Test that the handleChunkedResponse function correctly sends the header, boundary, and image data
void test_mjpeg_chunks(void)
{
    // max length of buffer provided that can be filled by handleChunkedResponse
    const size_t maxLen = 5000;
    QueueHandle_t mjpegQueue = xQueueCreate(2, sizeof(JpegImage));
    // add some data to the queue
    JpegImage img;
    img.length = 5500;
    img.data = (uint8_t *)ps_malloc(img.length);
    JpegImage img2;
    img2.length = 6500;
    img2.data = (uint8_t *)ps_malloc(img2.length);
    // valid jpegs are not needed for this test
    // we will check that data in buffer has been written to 0
    // to enshure that the right amount of fake jpeg has been written
    for (int i = 0; i < img.length; i++)
    {
        img.data[i] = 0;
        img2.data[i] = 0;
    }
    xQueueSend(mjpegQueue, &img, 0);
    xQueueSend(mjpegQueue, &img2, 0);

    // track how much for the current image has been sent etc
    MjpegStreamState state;

    // esp async web server provides the buffer and maxLen
    // here we write to the same buffer multiple times via handleChunkedResponse
    // esp async web server provides new buffers for each call
    // we are creating a buffer larger than maxLen to test that the function does not write more than maxLen
    uint8_t *buffer = (uint8_t *)ps_malloc(8000);
    // fill buffer with some data
    for (int i = 0; i < 6000; i++)
    {
        buffer[i] = 127;
    }
    // Header should be sent in first chunk
    size_t len = handleChunkedResponse(&state, mjpegQueue, buffer, maxLen, 0);
    TEST_ASSERT_EQUAL_UINT(36, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("\r\n--123456789000000000000987654321\r\n", buffer, 36);

    // Boundary should be sent in second chunk
    len = handleChunkedResponse(&state, mjpegQueue, buffer, maxLen, 0);
    TEST_ASSERT_EQUAL_UINT(50, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("Content-Type: image/jpeg\r\nContent-Length: 5500\r\n\r\n", buffer, 50);

    // Image should be sent in third chunk up to limit
    len = handleChunkedResponse(&state, mjpegQueue, buffer, maxLen, 0);

    TEST_ASSERT_EQUAL_UINT(maxLen, len);
    TEST_ASSERT_EACH_EQUAL_UINT8(0, buffer, maxLen);
    TEST_ASSERT_EACH_EQUAL_UINT8(127, buffer + maxLen, 1000);

    // reset buffer (normall we get a new buffer from esp async web server) since we are reusing the same buffer
    // the test must rest it to be sure to test the next chunk
    for (int i = 0; i < 6000; i++)
    {
        buffer[i] = 127;
    }
    // Rest of Image should be sent in 4th chunk up
    len = handleChunkedResponse(&state, mjpegQueue, buffer, maxLen, 0);

    TEST_ASSERT_EQUAL_UINT(500, len);
    TEST_ASSERT_EACH_EQUAL_UINT8(0, buffer, 500);

    // prepare for next image
    // header chunk
    len = handleChunkedResponse(&state, mjpegQueue, buffer, maxLen, 0);

    TEST_ASSERT_EQUAL_UINT(36, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("\r\n--123456789000000000000987654321\r\n", buffer, 36);

    // Boundary chunk
    len = handleChunkedResponse(&state, mjpegQueue, buffer, maxLen, 0);
    TEST_ASSERT_EQUAL_UINT(50, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("Content-Type: image/jpeg\r\nContent-Length: 6500\r\n\r\n", buffer, 50);
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mjpeg_chunks);
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
