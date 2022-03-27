#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <sqlite3.h> 
#include <sstream>  
#include <ctime>
#include <iomanip>
#include <iostream>
#include <pqxx/pqxx>

#include "SFA30x.h"
#include "ADS1115.h"
#include "SGP40.h"
#include "SEN5x.h"
#include "HDC2080.h"
#include "BMP280.h"
#include "VEML7700.h"
#include "PMS7003.h"
#include "i2c_helper.h"
#include "BME680/bme68x.h"
#include "BME680/bme68x_adapter.h"

#include "password.h"

#include "Display.h"
/*
	UNIT_INSIDE -- PMS7003, VEML7700, HDC2080, BMP280
	UNIT_OUTSIDE -- OLED, BMP280, SGP40, BME680, MQ9B(ADS1115), SFA30, Sen5x
				--  BMP280  P,T
			        SGP40   VOC_Index(0-500), 
					BME680  sIAQ, H,P,T, CO2eq,
					MQ9B    voltage
					SFA30   CH₂O ppb, H, T
					Sen5x PM0.5 PM1 PM2.5 H, T, VocIndex, NoxIndex(1-500,100=average)
*/

//#define UNIT_OUTSIDE
#define UNIT_INSIDE

#define db_password  PASSWORD

#ifdef UNIT_OUTSIDE
const char* DEVICEID = "103";
const int COMMIT_EVERY_MS=60 * 1000;
const int COMMIT_TO_ONLINE=10;
#endif

#ifdef UNIT_INSIDE
const char* DEVICEID = "104";
const int COMMIT_EVERY_MS=10 * 1000; 
const int COMMIT_TO_ONLINE=60;
#endif



#define SQL_DB_PATH  "inside_sensor.db"

void RUN_EVERY_MS(auto start, int duration_MS) 
{
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<float> diff = end - start;
	int sleep_time = duration_MS - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
	if(sleep_time>0)
		sleep_ms(sleep_time);
}

void temp_pressure_loop(CMA_Data *obj)
{
	while (1)
	{
		auto start = std::chrono::steady_clock::now();
		float t1=0, humidity, t2, pressure;
#ifdef UNIT_OUTSIDE
		read_from_hdc2080(&t1, &humidity);
#endif 
		read_from_BMP280(&t2, &pressure);
		if(t1==0) t1=t2;
		add_to_CMA(obj, (t1+t2)/2, humidity,pressure,0);
		RUN_EVERY_MS(start,250);
	}
}

void sfa_sgp_loop(CMA_Data *obj)
{
	while (1)
	{
		auto start = std::chrono::steady_clock::now();
		float temp, humidity, hcho;
		int voc;
		sfa3x_read(&hcho, &temp,&humidity);
		SGP40_read(temp,humidity,&voc);

		add_to_CMA(obj, temp, humidity, hcho, (float)voc);
		RUN_EVERY_MS(start,1000);
	}
}
void sen5x_loop(CMA_Data *obj, CMA_Data *obj2)
{
	while (1)
	{
		auto start = std::chrono::steady_clock::now();
		float pm1, pm2p5, hum, temp, VOC, NOX;
		SEN5x_read(&pm1, &pm2p5, &hum, &temp, &VOC, &NOX);

		add_to_CMA(obj, pm1, pm2p5, hum, 0);
		add_to_CMA(obj2, temp, VOC, NOX,0);
		RUN_EVERY_MS(start,1200);
	}
}
void ADS1115_loop(CMA_Data *obj)
{
	while (1)
	{
		float voltage;
		ads1115_read(&voltage);
		add_to_CMA(obj, voltage,0,0,0);
	}
}
void BME680_loop(CMA_Data *obj)
{
	return;
	while (1)
	{
		auto start = std::chrono::steady_clock::now();
		
		//add_to_CMA(obj, temp, humidity, hcho, (float)voc);
		RUN_EVERY_MS(start,1200);
	}
}
#ifdef UNIT_OUTSIDE
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
		int sleep_time = 300 - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
		if(sleep_time>0)
			sleep_ms(sleep_time);
	}
}
#endif 
int per_row_callback(void* string_item, int argc, char** argv, char** column_name)
{
	if(string_item==NULL)
		return -1;
	if(argc !=8)
		return -2;
	std::string *continuous_insert =  (std::string*)string_item;
	(*continuous_insert)+="INSERT INTO rpi_sensor(time, sensor_id, temperature, pressure, humidity, light, ppm1, ppm25, ppm10) ";
	(*continuous_insert)+="VALUES(";
	int ordering[9];
	for(int i=0;i<8;i++)
		ordering[i]=-1;
	ordering[1] = 55; //special
	for(int i=0;i<argc;i++)
	{
		if (strcmp(column_name[i], "iso_date")==0)
		{
			ordering[0]=i;
		}
		else if(strcmp(column_name[i], "temp")==0)
		{
			ordering[2]=i;
		}
		else if(strcmp(column_name[i], "pressure")==0)
		{
			ordering[3]=i;
		}
		else if(strcmp(column_name[i], "humidity")==0)
		{
			ordering[4]=i;
		}
		else if(strcmp(column_name[i], "lux")==0)
		{
			ordering[5]=i;
		}
		else if(strcmp(column_name[i], "ppm1")==0)
		{
			ordering[6]=i;
		}
		else if(strcmp(column_name[i], "ppm25")==0)
		{
			ordering[7]=i;
		}
		else if(strcmp(column_name[i], "ppm10")==0)
		{
			ordering[8]=i;
		}
		else
		{
			return -99; //bad
		}
	}
		for(int i=0;i< (8 + 1);i++)
		{
			if(ordering[i]==-1)
				return -98;
			
			if(i==0)
			{
				(*continuous_insert)+="\'";
				(*continuous_insert)+=argv[ordering[i]];
				(*continuous_insert)+="\'";
			}
			else if(i==1)
				(*continuous_insert)+=DEVICEID;
			else /*i!=1 or 0*/
				(*continuous_insert)+=argv[ordering[i]];
			if(i!=8)
				(*continuous_insert)+=",";
		}
	
	(*continuous_insert)+=") ON CONFLICT DO NOTHING;";
	return 0; //success
}

int main()
{
	if (display_init()!=0)
	{
		printf("Display init fail!\n");
		return -1;
	}

	if (setup_i2C() != 0)
	{
		printf("Failed to open I2c file\n");
		return -1;
	}

#ifdef UNIT_OUTSIDE
	CMA_Data temp_pressure_data, light_data, ppm_data;
#endif

#ifdef UNIT_INSIDE
	CMA_Data bmp280_data, sen5x_data_1,sen5x_data_2, sfa30_sgp40_data, bme680_data, ads1115_data;
#endif

#ifdef UNIT_OUTSIDE
	if (setup_hdc2080() != 0)
		return 1;
	printf("HDC2080 identified.\n");
	if (setup_VEML7700() != 0)
		return 3;
	printf("VEML7700 identified.\n");
	setup_PMS7003();
#endif 

  struct bme68x_dev gas_sensor;

    //gas_sensor.dev_id = BME68X_I2C_ADDR_HIGH;
    gas_sensor.intf = BME68X_I2C_INTF;
    gas_sensor.read = user_i2c_read;
    gas_sensor.write = user_i2c_write;
    gas_sensor.delay_us = user_delay_us;

    gas_sensor.amb_temp = 20;


	//COMMON
	if (setup_BMP280() != 0)
		return 2;
	printf("BMP280 identified.\n");

	//XXX - GET AMBIENT FROM BMP280 AND PASS TO BME68x

    int8_t rslt = BME68X_OK;
    rslt = bme68x_selftest_check(&gas_sensor);
	if(rslt==0)
		printf("BME680 identified and pass self-test\n");
	else{
		printf("BME680 FAIL SELFTEST: %i\n",rslt);
		return 5;
	}
	
	rslt = sfa3x_start();
	if(rslt==0)
		printf("SFA30 identified\n");
	else{
		printf("SFA30 start fail: %i\n",rslt);
		return 6;
	}
	rslt = ads1115_start();
	if(rslt==0)
		printf("ads1115 identified\n");
	else{
		printf("ads1115 start fail: %i\n",rslt);
		return 7;
	}
	rslt = SEN5x_start();
	if(rslt==0)
		printf("SEN5x identified\n");
	else{
		printf("SEN5x start fail: %i\n",rslt);
		return 7;
	}
	rslt = SGP40_start();
	if(rslt==0)
		printf("SGP40 identified\n");
	else{
		printf("SGP40 start fail: %i\n",rslt);
		return 8;
	}
	


	
	sqlite3* DB;
	int ret = sqlite3_open(SQL_DB_PATH, &DB);
	if(ret)
	{
		printf("Unable to open DB!\n");
		printf("Can't open database: %s\n", sqlite3_errmsg(DB));
		return -50;
	}
#ifdef UNIT_OUTSIDE
	const char* create_db = "CREATE TABLE IF NOT EXISTS sensors ("
							"iso_date TEXT NOT NULL, "
							"temp REAL NOT NULL, "
							"pressure REAL NOT NULL, "
							"humidity REAL NOT NULL, "
							"lux REAL NOT NULL, "
							"ppm1 REAL NOT NULL, "
							"ppm25 REAL NOT NULL, "
							"ppm10 REAL NOT NULL);";
#endif 
#ifdef UNIT_INSIDE
	const char* create_db = "CREATE TABLE IF NOT EXISTS sensors ("
							"iso_date TEXT NOT NULL, "
							"temp REAL NOT NULL, "
							"pressure REAL NOT NULL, "
							"humidity REAL NOT NULL, "
							"voc REAL NOT NULL, "
							"ppm1 REAL NOT NULL, "
							"ppm25 REAL NOT NULL, "
							"ppm10 REAL NOT NULL);";
#endif 
	char* messageError;
/* XXX

	ret = sqlite3_exec(DB, create_db, NULL, 0, &messageError);
	if (ret != SQLITE_OK) {
        printf("Error Create Table! %s\n",messageError);
        sqlite3_free(messageError);
    }
*/

	sleep_ms(1000); //give time for everything to reset

#ifdef UNIT_OUTSIDE
	std::thread lux_thread(light_loop, &light_data);
	std::thread ppm_thread(ppm_loop, &ppm_data);
#endif


	std::thread tmp_thread(temp_pressure_loop, &bmp280_data);
	std::thread sfa_thread(sfa_sgp_loop, &sfa30_sgp40_data);
	std::thread sen_thread(sen5x_loop, &sen5x_data_1,&sen5x_data_2);
	std::thread ads_thread(ADS1115_loop, &ads1115_data);
	std::thread bme_thread(BME680_loop, &bme680_data);
	float voltage;
	float pm1, pm2p5, hum_sen5x, temp_sen5x, VOC_sen5x, NOX;
	float temp_bmp280, press; 
	float temp_sfa, hum_sfa, hcho, voc_sgp;
	printf("temp_sen5x, temp_bmp280, temp_sfa, hum_sen5x, hum_sfa,voltage,VOC_sen5x,voc_sgp,NOX,press,hcho,pm1, pm2p5\n");
	while(1)
	{

		sleep_ms(3000); 
		remove_CMA(&bmp280_data,&temp_bmp280,NULL,&press, NULL);
		remove_CMA(&sfa30_sgp40_data,&temp_sfa,&hum_sfa,&hcho, &voc_sgp);
		remove_CMA(&sen5x_data_1,&pm1,&pm2p5,&hum_sen5x,NULL);
		remove_CMA(&sen5x_data_2,&temp_sen5x,&VOC_sen5x,&NOX, NULL);
		remove_CMA(&ads1115_data,&voltage,NULL,NULL,NULL);
		//remove_CMA(&bme680_data,NULL,NULL,NULL,NULL);
		printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",temp_sen5x, temp_bmp280, temp_sfa, hum_sen5x, hum_sfa,voltage,VOC_sen5x,voc_sgp,NOX,press,hcho,pm1,pm2p5);


	}
	/*	
	std::stringstream sql_transaction_string;
	//float temp, humidity, press, lux, ppm10, ppm25, ppm01;
	int commit=0;
	while (1)
	{
		sql_transaction_string.str("");
		sql_transaction_string << "INSERT INTO sensors VALUES(";
		sleep_ms(COMMIT_EVERY_MS); 

		remove_CMA(&bmp280_sgp40_data,&temp,&humidity,&press);
#ifdef UNIT_OUTSIDE
		remove_CMA(&light_data,&lux,NULLptr,NULLptr);
		remove_CMA(&ppm_data,&ppm10, &ppm25, &ppm01);
#endif
		std::time_t t= std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		sql_transaction_string << std::put_time(std::gmtime(&t), "\"%FT%TZ\"" );
		sql_transaction_string << ",";
		sql_transaction_string << temp << ",";
		sql_transaction_string << press << ",";
		sql_transaction_string << humidity << ",";
		sql_transaction_string << lux << ",";
		sql_transaction_string << ppm01 << ",";
		sql_transaction_string << ppm25 << ",";
		sql_transaction_string << ppm10 << ");";

		//std::cout << sql_transaction_string.str() << std::endl;
		ret = sqlite3_exec(DB, sql_transaction_string.str().c_str(), NULL, 0, &messageError);
		if (ret != SQLITE_OK) {
        	printf("Error INSERTING into Table! %s\n",messageError);
        	sqlite3_free(messageError);
			return -1;
    	}
		ret = sqlite3_db_cacheflush(DB);
		if (ret != SQLITE_OK) {
        	printf("Error FLUSHING into Table! %s\n",messageError);
        	sqlite3_free(messageError);
			return -3;
    	}
		fflush(NULL);
		commit++;
		if(false)
		//if(commit>=COMMIT_TO_ONLINE)
		{
			commit=0;
			try
			{
				std::string result_string = "";
				ret = sqlite3_exec(DB, "select * from sensors;", &per_row_callback, &result_string, &messageError);
				if (ret != SQLITE_OK) {
					printf("Error REMOVING from Table! %s\n",messageError);
					sqlite3_free(messageError);
					return -2;
				}
				pqxx::connection c("dbname=sensors host=database user=sensors_write password=" db_password);
				pqxx::work w(c);
				pqxx::result r = w.exec(result_string);
				w.commit();
				ret = sqlite3_exec(DB, "DELETE FROM sensors;", NULL, 0, &messageError);
				if (ret != SQLITE_OK) {
					printf("Error cleaning the table! %s\n",messageError);
					sqlite3_free(messageError);
					return -4;
				}

			}
			 catch (const std::exception &e)
			{
				printf("Error connecting to primary database! %s\n",e.what());
			}

		}
	}

	sqlite3_close(DB);
	*/
	return 0;
}