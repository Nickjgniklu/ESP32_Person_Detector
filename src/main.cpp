#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "wifikeys.h"
#include <FreeRTOS.h>
#include "camera_config.h"

typedef struct
{
  uint8_t *data; // Pointer to the data
  size_t length; // Length of the data
} JpegImage;
QueueHandle_t mjpegQueue;

AsyncWebServer server(80);
OV2640 camera;

// put function declarations here:
void notFound(AsyncWebServerRequest *request);
void setupWifi();
void setupServer();
void setupCamera();
void cameraTask(void *pvParameters);

void setup()
{
  // delay for 10 seconds to allow for serial monitor to connect
  delay(15000);
  ESP_LOGI("SETUP", "Starting setuo");
  mjpegQueue = xQueueCreate(4, sizeof(JpegImage));

  Serial.begin(115200);
  setupCamera();
  xTaskCreate(
      cameraTask,    // Task function
      "Camera Task", // Name of the task (for debugging purposes)
      2048,          // Stack size (bytes)
      NULL,          // Parameter to pass to the task
      1,             // Task priority
      NULL           // Task handle
  );
  setupWifi();
  setupServer();
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(1000);
}

// put function definitions here:
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setupCamera()
{
  ESP_LOGI("CAMERA", "Starting Camera");
  esp_err_t err = camera.init(esp32cam_ESPCam_config);
  if (ESP_OK != err)
  {
    ESP_LOGE("CAMERA", "Failed to start camera, will restart in 10s, error: %s", esp_err_to_name(err));
    delay(10000);

    // ESP.restart();
  }
  ESP_LOGI("CAMERA", "Started Camera");
}

void setupWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    ESP_LOGE("WIFI_SETUP", "WiFi Failed! will restart in 10 second.");

    delay(10000);
    // ESP.restart();
  }
  ESP_LOGI("WIFI_SETUP", "IP Address: %s", WiFi.localIP().toString().c_str());
  delay(5000);
}
size_t frame_index = 0;
bool send_boundary = true;
bool send_header = false;
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const int bdrLen = strlen(BOUNDARY);
uint8_t *currentSteamingImageBuffer;
size_t currentSteamingImageSize;
uint timeOfLastFrame = 0;
bool frame_available = false;

; // TODO gt this

void setupServer()
{
  server.on("/mjpeg", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              AsyncWebServerResponse *response = request->beginChunkedResponse("multipart/x-mixed-replace; boundary=123456789000000000000987654321", 
              [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
              {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //index equals the amount of bytes that have been already sent
  //You will be asked for more data until 0 is returned
  //Keep in mind that you can not delay or yield waiting for more data!
  //FYI the default maxLen seems to be around 6000 bytes
  //JPEGs seem to be around 5000 bytes
  //return RESPONSE_TRY_AGAIN results in a slow try again could possible
  //may beable to tune speed by sending header image and border all at once
  const size_t chunk_size = std::min(static_cast<size_t>(5000), maxLen);  //we need to read out of the image buffer
if(send_boundary){
    if (maxLen < bdrLen)
    {
    //hopefully this does not happen... but just incase try again later
      return RESPONSE_TRY_AGAIN;
    }
    ESP_LOGI("MJPEG STREAM", "Sending boundary");
    memcpy(buffer, BOUNDARY, bdrLen);
    send_boundary = false;
    return bdrLen;
  }
  JpegImage receivedItem;

  //if i can receive the queue track with globals
  if (!frame_available && xQueueReceive(mjpegQueue, &receivedItem, 0) == pdPASS) {
        ESP_LOGI("MJPEG STREAM", "time since last frame %d", millis() - timeOfLastFrame);
        timeOfLastFrame = millis();
        // Process the received item
        ESP_LOGI("MJPEG STREAM", "Found frame %d size", receivedItem.length);

        currentSteamingImageBuffer = receivedItem.data;
        currentSteamingImageSize = receivedItem.length;
        frame_available = true;
        send_header = true;
        // Remember to free the memory after processing
    }
  bool release_frame = false;
  if(send_header){
    ESP_LOGI("MJPEG STREAM", "Sending header for frame");

    char buf[32+43];

    //send the header
    const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";

    sprintf(buf, "%s%d\r\n\r\n", CTNTTYPE,currentSteamingImageSize);
    size_t header_length =strlen(buf);
    
    if(maxLen < header_length)
    {
      return RESPONSE_TRY_AGAIN;
    }
    memcpy(buffer, buf, header_length);
    send_header = false;
    return header_length;
  }
  //if we have a frame in the frame queue 
  if(frame_available)
  {
    //copy the frame into the buffer
    size_t copy_size;
    
    if(frame_index + chunk_size > currentSteamingImageSize)
    {
      copy_size = currentSteamingImageSize - frame_index;
      release_frame = true;

    }else
    {
      copy_size = chunk_size;
    }

    ESP_LOGI("MJPEG STREAM", "Sending frame %d bytes at index %d , max was %d", copy_size, frame_index, maxLen);
    memcpy(buffer, currentSteamingImageBuffer + frame_index, copy_size);
    frame_index += copy_size;

    if(release_frame)
    {
    ESP_LOGI("MJPEG STREAM", "Finished send frame cleaning up");

      frame_index = 0;
      send_boundary = true;
      frame_available = false;
      free(currentSteamingImageBuffer);

      //release the frame
    }
    //return the amount of bytes written
    return copy_size;
  }
  //send part of the frame
  //if all frame has been sent remove frame from queue and reset index

  // if we have no data return RESPONSE_TRY_AGAIN
  ESP_LOGI("MJPEG STREAM", "Nothing ready to send");

  return RESPONSE_TRY_AGAIN; });
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
   });
  server.onNotFound(notFound);
  server.begin();
}

void cameraTask(void *pvParameters)
{
  while (true)
  {
    if (uxQueueSpacesAvailable(mjpegQueue) > 0)
    {
      camera.run();

      size_t frame_size = camera.getSize();
      uint8_t *frame_buffer = (uint8_t *)ps_malloc(frame_size);
      memcpy(frame_buffer, camera.getfb(), frame_size);
      JpegImage image;
      image.data = frame_buffer;
      image.length = frame_size;
      if (xQueueSend(mjpegQueue, &image, 0) != pdPASS)
      {
        ESP_LOGE("CAMERA_TASK", "Failed to send frame to queue");
        free(frame_buffer);
      }
      else
      {
        ESP_LOGI("CAMERA_TASK", "Sent frame to queue");
      }
    }
    else
    {
      // dont check too often
      delay(10);
    }
  }
}

//
//
//