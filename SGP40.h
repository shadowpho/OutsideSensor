#pragma once


int SGP40_start();
int SGP40_read(float temp, float humidity, float* VOC);