#include "DB.h"

#include <iostream>
#include <pqxx/pqxx>
#include <sqlite3.h> 
#include <stdio.h>
#include <stdlib.h>
/*
	
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


    	ret = sqlite3_exec(DB, create_db, NULL, 0, &messageError);
	if (ret != SQLITE_OK) {
        printf("Error Create Table! %s\n",messageError);
        sqlite3_free(messageError);
    }


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

*/

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