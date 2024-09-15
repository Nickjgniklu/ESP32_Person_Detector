
// automatically generated file from training\esp32_transfer_learning.ipynb
#include "Model.h"

// We need to keep the data array aligned on some architectures.
#ifdef __has_attribute
#define HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define HAVE_ATTRIBUTE(x) 0
#endif
#if HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__))
#define DATA_ALIGN_ATTRIBUTE __attribute__((aligned(4)))
#else
#define DATA_ALIGN_ATTRIBUTE
#endif

unsigned const char  __model_tflite[] DATA_ALIGN_ATTRIBUTE = {

float model_quantization_scale = 0.00390625;
int model_quantization_zero_point = -128; 
float default_model_prediction_threshold = 0.30064174107142855;
