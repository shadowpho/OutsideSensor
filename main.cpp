#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <sqlite3.h> 

#include "HDC2080.h"
#include "BMP280.h"
#include "VEML7700.h"
#include "PMS7003.h"
#include "i2c_helper.h"


#include "password.h"
#define db_password  PASSWORD

const int DEVICEID = 103;
const int COMMIT_EVERY_MINUTE=1;
#define SQL_DB_PATH  "outside_sensor.db"


void temp_pressure_loop(CMA_Data *obj)
{
	while (1)
	{
		float t1, humidity, t2, pressure;
		read_from_hdc2080(&t1, &humidity);
		read_from_BMP280(&t2, &pressure);
		add_to_CMA(obj, (t1+t2)/2, humidity, pressure);
		sleep_ms(650);
	}
}
void light_loop(CMA_Data *obj)
{
	while (1)
	{
		auto start = std::chrono::steady_clock::now();
		float lux;
		read_from_VEML7700(&lux);
		add_to_CMA(obj, lux, 0, 0);
		auto end = std::chrono::steady_clock::now();
		std::chrono::duration<double> diff = end - start;
		int sleep_time = 1000 - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
		if(sleep_time>0)
			sleep_ms(sleep_time);
	}
}
void ppm_loop(CMA_Data *obj)
{
	while (1)
	{
		auto start = std::chrono::steady_clock::now();
		uint16_t ppm10, ppm25, ppm01;
		read_from_PMS(&ppm10, &ppm25, &ppm10);
		add_to_CMA(obj, (float)ppm10, (float)ppm25, (float) ppm01);
		auto end = std::chrono::steady_clock::now();
		std::chrono::duration<double> diff = end - start;
		int sleep_time = 1000 - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
		if(sleep_time>0)
			sleep_ms(sleep_time);
	}
}

int main()
{
	if (setup_i2C() != 0)
	{
		printf("Failed to open I2c file\n");
		return -1;
	}

	CMA_Data temp_pressure_data;
	CMA_Data light_data;
	CMA_Data ppm_data;

	if (setup_hdc2080() != 0)
		return 1;
	printf("HDC2080 identified.\n");
	if (setup_BMP280() != 0)
		return 2;
	printf("BMP280 identified.\n");
	if (setup_VEML7700() != 0)
		return 3;
	printf("VEML7700 identified.\n");
	setup_PMS7003();

	sqlite3* DB;
	int ret = sqlite3_open(SQL_DB_PATH, &DB);
	if(ret)
	{
		printf("Unable to open DB!\n");
		printf("Can't open database: %s\n", sqlite3_errmsg(DB));
		return -50;
	}

	sleep_ms(1000); //give time for everything to reset
	std::thread tmp_thread(temp_pressure_loop, &temp_pressure_data);
	std::thread lux_thread(light_loop, &light_data);
	std::thread ppm_thread(ppm_loop, &ppm_data);

	while (1)
	{

		float temp, press, humidity, lux, ppm10, ppm25, ppm01;
		remove_CMA(&temp_pressure_data,&temp,&humidity,&press);
		remove_CMA(&light_data,&lux,nullptr,nullptr);
		remove_CMA(&ppm_data,&ppm10, &ppm25, &ppm01);

		printf("%.2f,%.2f,%.2f,%.4f,%.1f\n", temp, press, humidity, lux, ppm10);
		sleep_ms(10 * 1000);
		
		fflush(NULL);
	}

	sqlite3_close(DB);
	return 0;
}