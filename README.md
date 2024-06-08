# A tensorflow person detector demo for esp32
This an example of using mobilenetv2 as basemodel (transfer learning) in a new model
that can run on the esp32s3
# features
- Live mjpeg stream
- Websocket for detection events
- Record images to sd card
- Person detection via esp32 tensorflow model

# Demo

# Hardware
ESP32S3 XIAO Sense
# Building the model
## Summary
1. Import load pretrained mobilenetv2 model
2. Create input layers that for 1 dimensional images
    (esp32 fmtrgb output a 1d buffer containing the image)
3. Add a new output layer for new predictions (person)/(no person)
4. Train on the coco dataset
5. Train further on a self collected dataset
6. quantize the model for esp32


see training\esp32_transfer_learning.ipynb for more details


# Compile firmware
Open the root folder of this project in Platform.io 
# Unit tests
`pio test`

