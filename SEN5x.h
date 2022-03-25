#pragma once

int SEN5x_start();
int SEN5x_read(float* pm1, float *pm2p5, float* hum, float* temp, float* VOC, float* NOX);