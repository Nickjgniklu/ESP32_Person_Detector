// automatically generated file from training\esp32_transfer_learning.ipynb
#include <tensorflow/lite/core/c/common.h>
#include <sys/types.h>
#include <string>
#include <fstream>
#include <vector>

#ifndef MODEL_H
#define MODEL_H

class Model {
public:
    Model();
    ~Model(); // Destructor declaration
    bool load_model(const std::string& path);
    const unsigned char* get_model() const;
    unsigned int get_length() const;
    float get_quantization_scale() const;
    int get_quantization_zero_point() const;
    float get_prediction_threshold() const;

private:
    uint8_t * model_data;
    unsigned int model_length;
    float quantization_scale;
    int quantization_zero_point;
    float prediction_threshold;
};

#endif // MODEL_H
