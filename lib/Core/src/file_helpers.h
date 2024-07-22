#pragma once
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

unsigned long getTotalBytes();
unsigned long getUsedBytes();
unsigned long getFreeBytes();
unsigned long getTotalFiles();