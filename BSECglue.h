#pragma once

#include "BME680/bme68x.h"

#include "i2c_helper.h"

int BSEC_BME_init();

int BSEC_BME_loop(float* temp, float* pressure, float* humidity,float* VOC );

int BSEC_BME_selftest();

int BSEC_desired_sleep_us();