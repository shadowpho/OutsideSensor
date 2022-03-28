#pragma once


//GUI
extern "C"{
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "GUI_BMPfile.h"
#include "OLED_1in5_rgb.h"
}


int display_init();

//temp_sen5x, temp_bmp280, temp_sfa, hum_sen5x, hum_sfa,voltage,VOC_sen5x,voc_sgp,NOX,press,hcho,pm1, pm2p5
int display_data(float temp, float humidity, float voltage, float VOC_S, float VOC_SGP, float NOX, float hcho, float pm1);

