#include "MjpegHandlers.h"

const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const int bdrLen = strlen(BOUNDARY);
size_t handleChunkedResponse(MjpegStreamState *state, QueueHandle_t mjpegQueue, uint8_t *buffer, size_t maxLen, size_t index)
{
    // Write up to "maxLen" bytes into "buffer" and return the amount written.
    // index equals the amount of bytes that have been already sent
    // You will be asked for more data until 0 is returned
    // Keep in mind that you can not delay or yield waiting for more data!
    // FYI the default maxLen seems to be around 6000 bytes
    // JPEGs seem to be around 5000 bytes
    // return RESPONSE_TRY_AGAIN results in a slow try again could possible
    // may beable to tune speed by sending header image and border all at once
    const size_t chunk_size = std::min(static_cast<size_t>(5000), maxLen); // we need to read out of the image buffer
    if (state->send_boundary)
    {
        if (maxLen < bdrLen)
        {
            // hopefully this does not happen... but just incase try again later
            return RESPONSE_TRY_AGAIN;
        }
        ESP_LOGI("MJPEG STREAM", "Sending boundary");
        memcpy(buffer, BOUNDARY, bdrLen);
        state->send_boundary = false;
        return bdrLen;
    }
    JpegImage receivedItem;

    if (!state->frame_available && xQueueReceive(mjpegQueue, &receivedItem, 0) == pdPASS)
    {
        ESP_LOGI("MJPEG STREAM", "time since last frame %d", millis() - state->timeOfLastFrame);
        state->timeOfLastFrame = millis();
        // Process the received item
        ESP_LOGI("MJPEG STREAM", "Found frame %d size", receivedItem.length);

        state->currentSteamingImageBuffer = receivedItem.data;
        state->currentSteamingImageSize = receivedItem.length;
        state->frame_available = true;
        state->send_header = true;
        // Remember to free the memory after processing
    }
    bool release_frame = false;
    if (state->send_header)
    {
        ESP_LOGI("MJPEG STREAM", "Sending header for frame");

        char buf[32 + 43];

        // send the header
        const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";

        sprintf(buf, "%s%d\r\n\r\n", CTNTTYPE, state->currentSteamingImageSize);
        size_t header_length = strlen(buf);

        if (maxLen < header_length)
        {
            return RESPONSE_TRY_AGAIN;
        }
        memcpy(buffer, buf, header_length);
        state->send_header = false;
        return header_length;
    }
    // if we have a frame in the frame queue
    if (state->frame_available)
    {
        // copy the frame into the buffer
        size_t copy_size;

        if (state->frame_index + chunk_size > state->currentSteamingImageSize)
        {
            copy_size = state->currentSteamingImageSize - state->frame_index;
            release_frame = true;
        }
        else
        {
            copy_size = chunk_size;
        }

        ESP_LOGI("MJPEG STREAM", "Sending frame %d bytes at index %d , max was %d", copy_size, state->frame_index, maxLen);
        memcpy(buffer, state->currentSteamingImageBuffer + state->frame_index, copy_size);
        state->frame_index += copy_size;

        if (release_frame)
        {
            ESP_LOGI("MJPEG STREAM", "Finished send frame cleaning up");

            state->frame_index = 0;
            state->send_boundary = true;
            state->frame_available = false;
            free(state->currentSteamingImageBuffer);

            // release the frame
        }
        // return the amount of bytes written
        return copy_size;
    }
    // send part of the frame
    // if all frame has been sent remove frame from queue and reset index

    // if we have no data return RESPONSE_TRY_AGAIN
    ESP_LOGI("MJPEG STREAM", "Nothing ready to send");

    return RESPONSE_TRY_AGAIN;
}

void handleMjpeg(AsyncWebServerRequest *request, QueueHandle_t mjpegQueue)
{
    // TODO create some object here to persist the state of the stream
    // to allow for multiple streams
    MjpegStreamState *state = new MjpegStreamState();

    AsyncWebServerResponse *response = request->beginChunkedResponse("multipart/x-mixed-replace; boundary=123456789000000000000987654321",
                                                                     [state, mjpegQueue](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                                                                     { return handleChunkedResponse(state, mjpegQueue, buffer, maxLen, index); });

    request->onDisconnect([state]()
                          {
                              ESP_LOGI("MJPEG STREAM", "Client disconnected");
                              delete state; });
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}
