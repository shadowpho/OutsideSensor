#pragma once

#include <cstdint>

int SGP40_start();
int SGP40_read(float temp, float humidity, int32_t* VOC);