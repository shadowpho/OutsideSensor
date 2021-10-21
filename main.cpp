#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <sqlite3.h> 
#include <sstream>  
#include <ctime>
#include <iomanip>
#include <iostream>
#include <pqxx/pqxx>

#include "HDC2080.h"
#include "BMP280.h"
#include "VEML7700.h"
#include "PMS7003.h"
#include "i2c_helper.h"


#include "password.h"
#define db_password  PASSWORD

const int DEVICEID = 103;
const int COMMIT_EVERY_MS=60 * 1000;
const int COMMIT_TO_ONLINE=10;

#define SQL_DB_PATH  "outside_sensor.db"


void temp_pressure_loop(CMA_Data *obj)
{
	while (1)
	{
		float t1, humidity, t2, pressure;
		read_from_hdc2080(&t1, &humidity);
		read_from_BMP280(&t2, &pressure);
		add_to_CMA(obj, (t1+t2)/2, humidity,pressure);
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
		int sleep_time = 300 - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
		if(sleep_time>0)
			sleep_ms(sleep_time);
	}
}
int sqlite3_callback(void* string_item, int argc, char** argv, char** column_name)
{
	if(string_item==NULL)
		return -1;
	if(argc !=8)
		return -2;
	std::string continuous_insert = * (std::string*)string_item;
	string_item+="INSERT INTO rpi_sensor(time, sensor_id, temperature, pressure, humidity, light, ppm1, ppm25, ppm10)";
	string_item+="VALUES (";
	int ordering[9];
	for(int i=0;i<8;i++)
		ordering[i]=-1;

	for(int i=0;i<argc;i++)
	{
		if (strcmp(column_name[i], "iso_date")==0)
		{
			ordering[0]=i;
		}
		else if(strcmp(column_name[i], "temp")==0)
		{
			ordering[1]=i;
		}
		else if(strcmp(column_name[i], "pressure")==0)
		{
			ordering[2]=i;
		}
		else if(strcmp(column_name[i], "humidity")==0)
		{
			ordering[3]=i;
		}
		else if(strcmp(column_name[i], "lux")==0)
		{
			ordering[4]=i;
		}
		else if(strcmp(column_name[i], "ppm1")==0)
		{
			ordering[5]=i;
		}
		else if(strcmp(column_name[i], "ppm25")==0)
		{
			ordering[6]=i;
		}
		else if(strcmp(column_name[i], "ppm10")==0)
		{
			ordering[7]=i;
		}
		else
		{
			return -99; //bad
		}
	}
		for(int i=0;i<8;i++)
		{
			if(ordering[i]==-1)
				return -98;
			string_item+=argv[ordering[i]];
		}
	
	string_item+=") ON CONFLICT DO NOTHING;";
	return 0; //success
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
	const char* create_db = "CREATE TABLE IF NOT EXISTS sensors ("
							"iso_date TEXT NOT NULL, "
							"temp REAL NOT NULL, "
							"pressure REAL NOT NULL, "
							"humidity REAL NOT NULL, "
							"lux REAL NOT NULL, "
							"ppm1 REAL NOT NULL, "
							"ppm25 REAL NOT NULL, "
							"ppm10 REAL NOT NULL);";
	char* messageError;
	ret = sqlite3_exec(DB, create_db, NULL, 0, &messageError);
	if (ret != SQLITE_OK) {
        printf("Error Create Table! %s\n",messageError);
        sqlite3_free(messageError);
    }


	sleep_ms(1000); //give time for everything to reset
	std::thread tmp_thread(temp_pressure_loop, &temp_pressure_data);
	std::thread lux_thread(light_loop, &light_data);
	std::thread ppm_thread(ppm_loop, &ppm_data);

	std::stringstream sql_transaction_string;
	float temp, humidity, press, lux, ppm10, ppm25, ppm01;
	int commit=0;
	while (1)
	{
		sql_transaction_string.str("");
		sql_transaction_string << "INSERT INTO sensors VALUES(";
		sleep_ms(COMMIT_EVERY_MS); //1x a minute

		remove_CMA(&temp_pressure_data,&temp,&humidity,&press);
		remove_CMA(&light_data,&lux,nullptr,nullptr);
		remove_CMA(&ppm_data,&ppm10, &ppm25, &ppm01);

		std::time_t t= std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		sql_transaction_string << std::put_time(std::gmtime(&t), "\"%FT%T\"" );
		sql_transaction_string << ",";
		sql_transaction_string << temp << ",";
		sql_transaction_string << press << ",";
		sql_transaction_string << humidity << ",";
		sql_transaction_string << lux << ",";
		sql_transaction_string << ppm01 << ",";
		sql_transaction_string << ppm25 << ",";
		sql_transaction_string << ppm10 << ");";

		std::cout << sql_transaction_string.str() << std::endl;
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
		if(commit>=COMMIT_TO_ONLINE)
		{
			commit=0;
			try
			{
				sql_transaction_string.str("select * from sensors;");
				std::string result_string = "";
				ret = sqlite3_exec(DB, sql_transaction_string.str().c_str(), &sqlite3_callback, &result_string, &messageError);
				if (ret != SQLITE_OK) {
					printf("Error REMOVING from Table! %s\n",messageError);
					sqlite3_free(messageError);
					return -2;
				}
				pqxx::connection c("dbname=sensors host=database user=sensors_write password=" db_password);
				pqxx::work w(c);
				pqxx::result r = w.exec(result_string);
				w.commit();
				sql_transaction_string.str("DELETE FROM sensors;");
				ret = sqlite3_exec(DB, sql_transaction_string.str().c_str(), NULL, 0, &messageError);
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
	return 0;
}