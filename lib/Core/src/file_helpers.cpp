#include "file_helpers.h"
#define TAG "FILE_HELPERS"

//TODO: No really sure what happens if the SD card is not initialized
//currently the save task inits the sd card

unsigned long getTotalFiles()
{
  File root = SD.open("/");
  if (!root)
  {
    ESP_LOGI(TAG, "Failed to open directory");
    return 0;
  }
  if (!root.isDirectory())
  {
    ESP_LOGI(TAG, "Not a directory");
    return 0;
  }

  File file = root.openNextFile();
  u64_t count = 0;
  while (file)
  {
    if (!file.isDirectory())
    {
      count++;
    }
    file = root.openNextFile();
  }
  ESP_LOGI(TAG, "File count: %d", count);
  return count;
}
unsigned long getTotalBytes()
{

  return SD.cardSize();
}
unsigned long getUsedBytes()
{
  return SD.usedBytes();
}
unsigned long getFreeBytes()
{
  return SD.totalBytes() - SD.usedBytes();
}
