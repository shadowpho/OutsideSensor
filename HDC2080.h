#pragma once

#define ADDR 0x40

//reset and block!
int setup_hdc2080();

//blocking read!
int read_from_hdc2080(float* temp, float *humid);