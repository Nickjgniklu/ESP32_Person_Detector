#pragma once
#include <stdio.h>
#include <time.h>
typedef struct
{
    uint8_t *data; // Pointer to the data
    size_t length; // Length of the data
} JpegImage;