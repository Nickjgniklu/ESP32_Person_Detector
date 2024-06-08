#include "file_helpers.h"

#include <ArduinoJson.h>

#define TAG "MESSAGES"

String systemInfoMessage()
{
  JsonDocument json;
  json["responseType"] = "systemInfo";
  json["heapSize"] = ESP.getFreeHeap();
  json["sdkVersion"] = ESP.getSdkVersion();
  json["chipModel"] = ESP.getChipModel();
  json["freeHeap"] = ESP.getFreeHeap();
  json["freePsram"] = ESP.getFreeHeap();
  json["psRamSize"] = ESP.getPsramSize();
  json["uptimeMs"] = millis();

  String output;

  serializeJson(json, output);
  return output;
}
String sdInfoMessage()
{
  JsonDocument json;
  json["responseType"] = "sdInfo";
  json["totalBytes"] = getTotalBytes();
  json["usedBytes"] = getUsedBytes();
  json["freeBytes"] = getFreeBytes();
  String output;

  serializeJson(json, output);
  return output;
}
String predictionMessage(String className, float probability)
{
  JsonDocument json;
  json["responseType"] = "prediction";
  json["probability"] = probability;
  json["prediction"] = className;
  String output;

  serializeJson(json, output);
  return output;
}
