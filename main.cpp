#include <stdio.h>
#include <stdlib.h>

#include "HDC2080.h"
#include "BMP280.h"
#include "VEML7700.h"
#include "PMS7003.h"
#include "i2c_helper.h"

int main(){
	if(setup_i2C()!=0)
	{
		printf("Failed to open I2c file\n");
		return -1;
	}

    float temperature, humidity, lux, t2, p2; 
	uint16_t pm1, pm25, pm10;

	if(setup_hdc2080() != 0 ) return 1; 
	printf("HDC2080 identified.\n");
	if(setup_BMP280() != 0 ) return 2;
	printf("BMP280 identified.\n");
	if(setup_VEML7700() !=0) return 3;
	printf("VEML7700 identified.\n");
	setup_PMS7003();
	
	while(1)
	{
	read_from_hdc2080(&temperature, &humidity);
	read_from_VEML7700(&lux);
	read_from_BMP280(&t2,&p2);
	printf("%.2f,%.2f,%.2f,%.2f,%.4f\n", temperature,t2,p2,  humidity, lux);

	read_from_PMS(&pm10, &pm25, &pm1);

	sleep_ms(1000);
	}
    return 0;
}