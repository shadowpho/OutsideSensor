#pragma once


#define BMP_ADDR 0x76

//reset and block!
int setup_BMP280();

//blocking read!
int read_from_BMP280(float* temp, float *pressure);