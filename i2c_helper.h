#pragma once

#include <cstdint>
#include <mutex>

struct CMA_Data{
    CMA_Data() : data_mutex(), CMA_value(0), num_of_samples(0);
    std::mutex data_mutex;  //must take this before changing values below
    float CMA_value;        //Cumilitative moving average
    uint32_t num_of_samples;
};

void add_to_CMA(float val);
float remove_CMA();

void sleep_ms(uint32_t sleep_ms);

int communicate_I2C(uint8_t device_address, bool write_comm, uint8_t register_address, uint8_t *recv_buff, int8_t num_of_bytes);

int setup_i2C();