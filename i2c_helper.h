#pragma once

#include <cstdint>
#include <mutex>

struct CMA_Data
{
    std::mutex data_mutex; //must take this before changing values below
    double CMA_value1 = 0;   //moving average val1
    double CMA_value2 = 0;   //moving average val2 
    double CMA_value3 = 0;   //moving average val3
    uint32_t num_of_samples = 0;
};

void add_to_CMA(CMA_Data *struct_data, float val1, float val2, float val3);
void remove_CMA(CMA_Data *struct_data, float* val1, float* val2, float* val3);

void sleep_ms(uint32_t sleep_ms);

int communicate_I2C(uint8_t device_address, bool write_comm, uint8_t register_address, uint8_t *recv_buff, int8_t num_of_bytes);

int setup_i2C();