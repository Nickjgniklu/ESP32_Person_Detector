#include <OV2640.h>
#ifdef CAMERA_MODEL_SEEED_XIAO_ESP32S3
#define CAMERA_PIN_PWDN -1
#define CAMERA_PIN_RESET -1

#define CAMERA_PIN_VSYNC 38
#define CAMERA_PIN_HREF 47
#define CAMERA_PIN_PCLK 13
#define CAMERA_PIN_XCLK 10

#define CAMERA_PIN_SIOD 40
#define CAMERA_PIN_SIOC 39

#define CAMERA_PIN_D0 15
#define CAMERA_PIN_D1 17
#define CAMERA_PIN_D2 18
#define CAMERA_PIN_D3 16
#define CAMERA_PIN_D4 14
#define CAMERA_PIN_D5 12
#define CAMERA_PIN_D6 11
#define CAMERA_PIN_D7 48
#endif

#ifdef CAMERA_MODEL_AI_THINKER
#define CAMERA_PIN_PWDN 32
#define CAMERA_PIN_RESET -1
#define CAMERA_PIN_XCLK 0
#define CAMERA_PIN_SIOD 26
#define CAMERA_PIN_SIOC 27
#define CAMERA_PIN_D7 35
#define CAMERA_PIN_D6 34
#define CAMERA_PIN_D5 39
#define CAMERA_PIN_D4 36
#define CAMERA_PIN_D3 21
#define CAMERA_PIN_D2 19
#define CAMERA_PIN_D1 18
#define CAMERA_PIN_D0 5
#define CAMERA_PIN_VSYNC 25
#define CAMERA_PIN_HREF 23
#define CAMERA_PIN_PCLK 22
#endif

camera_config_t esp32cam_ESPCam_config{

    .pin_pwdn = CAMERA_PIN_PWDN,
    .pin_reset = CAMERA_PIN_RESET,
    .pin_xclk = CAMERA_PIN_XCLK,
    .pin_sscb_sda = CAMERA_PIN_SIOD,
    .pin_sscb_scl = CAMERA_PIN_SIOC,
    .pin_d7 = CAMERA_PIN_D7,
    .pin_d6 = CAMERA_PIN_D6,
    .pin_d5 = CAMERA_PIN_D5,
    .pin_d4 = CAMERA_PIN_D4,
    .pin_d3 = CAMERA_PIN_D3,
    .pin_d2 = CAMERA_PIN_D2,
    .pin_d1 = CAMERA_PIN_D1,
    .pin_d0 = CAMERA_PIN_D0,
    .pin_vsync = CAMERA_PIN_VSYNC,
    .pin_href = CAMERA_PIN_HREF,
    .pin_pclk = CAMERA_PIN_PCLK,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_SVGA,
    .jpeg_quality = 9,
    .fb_count = 2};