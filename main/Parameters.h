#pragma once

#include <string.h>
#include <functional>
#include <esp_err.h>
#include <esp_crc.h>
#include <nvs_flash.h>

template<typename T>
class Parameters {
public:
    typedef std::function<void(T*)> clearcb_t;

    Parameters() : _clearcb(nullptr), _nvs(0) {}
    ~Parameters() {
        end();
    }

    esp_err_t begin();
    void end();
    bool check() const;
    operator bool() const {
        return check();
    }
    T* operator ->() {
        return &_blob.data;
    }
    void clear();
    esp_err_t commit();
    void onClear(clearcb_t cb) {
        _clearcb = cb;
    }

protected:
    static constexpr uint16_t SIGN = 0xA3C5;

    clearcb_t _clearcb;
    nvs_handle_t _nvs;
    struct __attribute__((__packed__)) paramblob_t {
        uint16_t sign;
        uint16_t crc;
        T data;
    } _blob;
};

#define NVS_NAMESPACE   "Parameters"
#define NVS_BLOBNAME    "Data"

template<typename T>
esp_err_t Parameters<T>::begin() {
    esp_err_t result;

    result = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &_nvs);
    if (result == ESP_OK) {
        size_t size = sizeof(paramblob_t);

        result = nvs_get_blob(_nvs, NVS_BLOBNAME, &_blob, &size);
        if ((result == ESP_OK) || (result == ESP_ERR_NVS_NOT_FOUND) || (result == ESP_ERR_NVS_INVALID_LENGTH)) {
            if ((result != ESP_OK) || (! check()))
                clear();
            return ESP_OK;
        }
        nvs_close(_nvs);
    }
    _nvs = 0;
    return result;
}

template<typename T>
void Parameters<T>::end() {
    if (_nvs) {
        if (! check())
            commit();
        nvs_close(_nvs);
        _nvs = 0;
    }
}

template<typename T>
bool Parameters<T>::check() const {
    return (_blob.sign == SIGN) && (_blob.crc == esp_crc16_le(0xFFFF, (uint8_t*)&_blob.data, sizeof(T)));
}

template<typename T>
void Parameters<T>::clear() {
    memset(&_blob.data, 0, sizeof(T));
    if (_clearcb)
        _clearcb(&_blob.data);
}

template<typename T>
esp_err_t Parameters<T>::commit() {
    esp_err_t result = ESP_ERR_INVALID_ARG;

    if (_nvs) {
        _blob.sign = SIGN;
        _blob.crc = esp_crc16_le(0xFFFF, (uint8_t*)&_blob.data, sizeof(T));
        result = nvs_set_blob(_nvs, NVS_BLOBNAME, &_blob, sizeof(paramblob_t));
    }
    return result;
}
