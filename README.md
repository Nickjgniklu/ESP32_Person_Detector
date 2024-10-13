# A TensorFlow Person Detector Demo for ESP32

This is an example of using MobileNetV2 as a base model (transfer learning) in a new model that can run on the ESP32S3.

## Features

- Live MJPEG stream
- WebSocket for detection events
- Record images to SD card
- Person detection via ESP32 TensorFlow model

## Demo

![Person Detector Demo](demo_files/person_detector_demo.gif)

## Hardware

ESP32S3 XIAO Sense

## Building the Model

### Summary

1. Import and load the pretrained MobileNetV2 model.
2. Create input layers for 1-dimensional images (ESP32 FMTRGB outputs a 1D buffer containing the image).
3. Add a new output layer for new predictions (person/no person).
4. Train on the COCO dataset.
5. Train further on a self-collected dataset.
6. Quantize the model for ESP32.

See `training/esp32_transfer_learning.ipynb` for more details.

## Compile Firmware

Open the root folder of this project in Platform.io.

## Unit Tests

Run the following command to execute unit tests:

```sh
pio test