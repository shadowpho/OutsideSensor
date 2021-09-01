#pragma once

#include <cstdint>
#include <mutex>

struct CMA_Data
{
    std::mutex data_mutex; //must take this before changing values below
    double CMA_value = 0;   //moving average
    uint32_t num_of_samples = 0;
};

void add_to_CMA(CMA_Data *struct_data, float val);
float remove_CMA(CMA_Data *struct_data);

void sleep_ms(uint32_t sleep_ms);

int communicate_I2C(uint8_t device_address, bool write_comm, uint8_t register_address, uint8_t *recv_buff, int8_t num_of_bytes);

int setup_i2C();