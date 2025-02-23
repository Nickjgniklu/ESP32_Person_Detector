#include "Model.h"
#include <SPIFFS.h>
#include "esp_log.h"

#define LOCAL_TAG "MODEL"
Model::Model() : model_data(nullptr), model_length(0), quantization_scale(0.00390625f), quantization_zero_point(-128), prediction_threshold(0.30064174107142855f) {
    // if (!SPIFFS.begin(true)) {
    //     ESP_LOGE(LOCAL_TAG, "An Error has occurred while mounting SPIFFS");
    // }
}

bool Model::load_model(const std::string& path) {
       File file = SPIFFS.open(path.c_str(), FILE_READ);
    if (!file) {
        ESP_LOGE(LOCAL_TAG, "Failed to open file for reading");
        return false;
    }

    model_length = file.size();
    model_data = (uint8_t*)ps_malloc(model_length);
    if (!model_data) {
        ESP_LOGE(LOCAL_TAG, "Failed to allocate PSRAM");
        file.close();
        return false;
    }

    if (file.read(model_data, model_length) != model_length) {
        ESP_LOGE(LOCAL_TAG, "Failed to read complete file");
        free(model_data);
        model_data = nullptr;
        model_length = 0;
        file.close();
        return false;
    }

    file.close();
    return true;
}

const unsigned char* Model::get_model() const {
    return model_data;
}

unsigned int Model::get_length() const {
    return model_length;
}

float Model::get_quantization_scale() const {
    return quantization_scale;
}

int Model::get_quantization_zero_point() const {
    return quantization_zero_point;
}

float Model::get_prediction_threshold() const {
    return prediction_threshold;
}

Model::~Model() {
    if (model_data) {
        free(model_data);
    }
}
