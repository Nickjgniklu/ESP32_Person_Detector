#pragma once
#include <stdio.h>
struct MjpegStreamState
{
    size_t frame_index = 0;
    bool send_boundary = true;
    bool send_header = false;

    uint8_t *currentSteamingImageBuffer;
    size_t currentSteamingImageSize;
    uint timeOfLastFrame = 0;
    bool frame_available = false;
    // Add other members...
};