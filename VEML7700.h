#pragma once


#define VEML7700_ADDR 0x10

//reset and block!
int setup_VEML7700();

//blocking read!
int read_from_VEML7700(float* lux);