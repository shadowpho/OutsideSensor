#pragma once

#include <cstdint>


int communicate_I2C(uint8_t device_address,bool write_comm, uint8_t register_address, uint8_t* recv_buff, int8_t num_of_bytes);

int setup_i2C();