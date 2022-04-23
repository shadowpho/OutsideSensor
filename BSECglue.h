#pragma once

#include "BME680/bme68x.h"

#define INVALID_VALUE -999.9

int BSEC_BME_init();

int BSEC_BME_loop(float* temp, float* pressure, float* humidity,float* VOC );

int BSEC_BME_selftest();

int BSEC_desired_sleep_us();