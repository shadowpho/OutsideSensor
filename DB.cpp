#include "DB.h"

#define UNIT_INSIDE 1

#include <ctime>
#include <iostream>
#include <pqxx/pqxx>
#include <sqlite3.h> 
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <iomanip>

#ifdef UNIT_OUTSIDE
#define SQL_DB_PATH "outside_sensor.db"
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
#define SQL_DB_PATH "inside_sensor.db"
	const char* create_db = "CREATE TABLE IF NOT EXISTS sensors ("
							"iso_date TEXT NOT NULL, "
							"temp REAL NOT NULL, "
							"VOC REAL NOT NULL, "
							"methane REAL NOT NULL, "
							"humidity REAL NOT NULL, "
							"pressure REAL NOT NULL, "
							"NOX REAL NOT NULL, "
							"HCHO REAL NOT NULL, "
							"pm1 REAL NOT NULL);";
#endif 

void record_to_db(float real_temp, float real_VOC, float voltage_methane, float real_hum, float real_pressure,  float NOX, float hcho, float pm1)
{
	sqlite3* DB;
	int ret = sqlite3_open(SQL_DB_PATH, &DB);
	if(ret)
	{
		printf("Unable to open DB!\n");
		printf("Can't open database: %s\n", sqlite3_errmsg(DB));
		return;
	}
	char* messageError;
    ret = sqlite3_exec(DB, create_db, NULL, 0, &messageError);
	if (ret != SQLITE_OK) {
        printf("Error Create Table! %s\n",messageError);
        sqlite3_free(messageError);
		return; 
    }
	std::stringstream sql_transaction_string;
	sql_transaction_string.str("");
	sql_transaction_string << "INSERT INTO sensors VALUES(";
	std::time_t t= std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		sql_transaction_string << std::put_time(std::gmtime(&t), "\"%FT%TZ\"" ) << ",";
		sql_transaction_string << real_temp << ",";
		sql_transaction_string << real_VOC << ",";
		sql_transaction_string << voltage_methane << ",";
		sql_transaction_string << real_hum << ",";
		sql_transaction_string << real_pressure << ",";
		sql_transaction_string << NOX << ",";
		sql_transaction_string << hcho << ",";
		sql_transaction_string << pm1 << ");";
		//std::cout << sql_transaction_string.str() << std::endl;
		ret = sqlite3_exec(DB, sql_transaction_string.str().c_str(), NULL, 0, &messageError);
		if (ret != SQLITE_OK) {
        	printf("Error INSERTING into Table! %s\n",messageError);
        	sqlite3_free(messageError);
			return;
    	}
		ret = sqlite3_db_cacheflush(DB);
		if (ret != SQLITE_OK) {
        	printf("Error FLUSHING into Table! %s\n",messageError);
        	sqlite3_free(messageError);
			return;
    	}
}
/*
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
			else //i!=1 or 0
				(*continuous_insert)+=argv[ordering[i]];
			if(i!=8)
				(*continuous_insert)+=",";
		}

	(*continuous_insert)+=") ON CONFLICT DO NOTHING;";
	return 0; //success
}
*/