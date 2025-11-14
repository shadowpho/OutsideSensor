#include "config.h"
#define db_password PASSWORD5a

#ifdef UNIT_OUTSIDE
const char* DEVICEID = "103";
const int COMMIT_EVERY_MS = 60 * 1000; //commit every minute
const int SAVE_EVERY_LOOP = 10;
#endif

#ifdef UNIT_INSIDE
const char* DEVICEID = "104";
const int CAPTURE_EVERY_MS = 10 * 1000;
const int SAVE_EVERY_LOOP = 6*10;
#endif



#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

#include "ADS1115.h"
#include "BMP280.h"
#include "BSECglue.h"
#include "HDC2080.h"
#include "PMS7003.h"
#include "SEN5x.h"
#include "SFA30x.h"
#include "SGP40.h"
#include "VEML7700.h"
#include "i2c_helper.h"

//#include "password.h"
#include "Display.h"
#include "DB.h"


using std::chrono::operator""ms;

/*
        UNIT_OUTSIDE -- PMS7003, VEML7700, HDC2080, BMP280
        UNIT_INSIDE -- OLED, BMP280, SGP40, BME680, MQ9B(ADS1115), SFA30, Sen5x, 
                                --  BMP280  P,T
                                        SGP40   VOC_Index(0-500),
                                        BME680  sIAQ, H,P,T, CO2eq,
                                        MQ9B    voltage
                                        SFA30   CH₂O ppb, H, T
                                        Sen5x PM0.5 PM1 PM2.5 H, T, VocIndex, NoxIndex(1-500,100=average)
*/

void RUN_EVERY_MS(auto start, int duration_MS)
{
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<float> diff = end - start;
  int sleep_time = duration_MS - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
  if (sleep_time > 0) sleep_ms(sleep_time);
}

void temp_pressure_loop(CMA_Data* obj)
{
  while (1) {
    auto start = std::chrono::steady_clock::now();
    float t1 = 0, humidity, t2, pressure;
#ifdef UNIT_OUTSIDE
    read_from_hdc2080(&t1, &humidity);
#endif
    read_from_BMP280(&t2, &pressure);
    if (t1 == 0) t1 = t2;
    add_to_CMA(obj, (t1 + t2) / 2, humidity, pressure, 0);
    std::this_thread::sleep_until(start + 250ms);
  }
}

void sfa_sgp_loop(CMA_Data* obj)
{
  while (1) {
    auto start = std::chrono::steady_clock::now();
    float temp, humidity, hcho;
    int voc;
    sfa3x_read(&hcho, &temp, &humidity);
    SGP40_read(temp - 6, humidity + 6, &voc);

    add_to_CMA(obj, temp, humidity, hcho, (float)voc);
    RUN_EVERY_MS(start, 1000);
  }
}
void sen5x_loop(CMA_Data* obj, CMA_Data* obj2)
{
  while (1) {
    auto start = std::chrono::steady_clock::now();
    float pm1, pm2p5, hum, temp, VOC, NOX;
    SEN5x_read(&pm1, &pm2p5, &hum, &temp, &VOC, &NOX);

    add_to_CMA(obj, pm1, pm2p5, hum, 0);
    add_to_CMA(obj2, temp, VOC, NOX, 0);
    RUN_EVERY_MS(start, 1200);
  }
}
void ADS1115_loop(CMA_Data* obj)
{
  while (1) {
    float voltage;
    ads1115_read(&voltage);
    add_to_CMA(obj, voltage, 0, 0, 0);
  }
}
void BME680_loop(CMA_Data* obj, CMA_Data* obj2)
{
  BSEC_BME_init();
  float temp, pressure, humidity, VOC;
  while (1) {
    int ret = BSEC_BME_loop(&temp, &pressure, &humidity, &VOC);
    if (ret != 0) { printf("BME/BSEC LOOP FAIL!!! %i\n", ret); }
    if(ret==100) {BSEC_BME_init();}
    if (!std::isnan(temp)) add_to_CMA(obj, temp, pressure, humidity, 0);
    if (!std::isnan(VOC)) add_to_CMA(obj2, VOC, 0, 0, 0);
    sleep_us(BSEC_desired_sleep_us());
  }
}
#ifdef UNIT_OUTSIDE
void light_loop(CMA_Data* obj)
{
  while (1) {
    auto start = std::chrono::steady_clock::now();
    float lux;
    read_from_VEML7700(&lux);
    add_to_CMA(obj, lux, 0, 0);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    int sleep_time = 1000 - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
    if (sleep_time > 0) sleep_ms(sleep_time);
  }
}

void ppm_loop(CMA_Data* obj)
{
  while (1) {
    auto start = std::chrono::steady_clock::now();
    uint16_t ppm10, ppm25, ppm01;
    read_from_PMS(&ppm10, &ppm25, &ppm10);
    add_to_CMA(obj, (float)ppm10, (float)ppm25, (float)ppm01);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    int sleep_time = 300 - std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
    if (sleep_time > 0) sleep_ms(sleep_time);
  }
}
#endif

int main()
{


  if (setup_i2C() != 0) {
    printf("Failed to open I2c file\n");
    return -1;
  }

#ifdef UNIT_OUTSIDE
  CMA_Data bmp280_data, light_data, ppm_data;
  if (setup_hdc2080() != 0) return 1;
    printf("HDC2080 identified.\n");
  if (setup_VEML7700() != 0) return 3;
    printf("VEML7700 identified.\n");
  setup_PMS7003();
#endif

#ifdef UNIT_INSIDE
  CMA_Data bmp280_data, sen5x_data_1, sen5x_data_2, sfa30_sgp40_data, bme680_dat1, bme680_dat2, ads1115_data;
    if (display_init() != 0) {
    printf("Display init fail!\n");
    return -1;
  }
#endif
  // COMMON
  int8_t rslt = 0;
  if (setup_BMP280() != 0) return 2;
  printf("BMP280 identified.\n");
#ifdef UNIT_INSIDE
  rslt = sfa3x_start();
  if (rslt == 0) printf("SFA30 identified\n");
  else {
    printf("SFA30 start fail: %i\n", rslt);
    return 6;
  }
  rslt = ads1115_start();
  if (rslt == 0) printf("ads1115 identified\n");
  else {
    printf("ads1115 start fail: %i\n", rslt);
    return 7;
  }
  rslt = SEN5x_start();
  if (rslt == 0) printf("SEN5x identified\n");
  else {
    printf("SEN5x start fail: %i\n", rslt);
    return 7;
  }
  rslt = SGP40_start();
  if (rslt == 0) printf("SGP40 identified\n");
  else {
    printf("SGP40 start fail: %i\n", rslt);
    return 8;
  }

  rslt = BSEC_BME_selftest();
  if (rslt == 0) printf("BME680 identified and pass self-test\n");
  else {
    printf("BME680 FAIL SELFTEST: %i\n", rslt);
    return 5;
  }
#endif

  sleep_ms(1000); // give time for everything to reset
  openDB();
  atexit(closeDB);

#ifdef UNIT_OUTSIDE
  std::thread lux_thread(light_loop, &light_data);
  std::thread ppm_thread(ppm_loop, &ppm_data);
#endif

  //common
  std::thread tmp_thread(temp_pressure_loop, &bmp280_data);

  #ifdef UNIT_INSIDE
  std::thread sfa_thread(sfa_sgp_loop, &sfa30_sgp40_data);
  std::thread sen_thread(sen5x_loop, &sen5x_data_1, &sen5x_data_2);
  std::thread ads_thread(ADS1115_loop, &ads1115_data);
  std::thread bme_thread(BME680_loop, &bme680_dat1, &bme680_dat2);
    printf("temp_sen5x, temp_bmp280, temp_sfa,bme680_t, hum_sen5x, hum_sfa,bme680_h, "
         "voltage,VOC_sen5x,voc_sgp,bme680_voc,NOX,press,bme680_p,hcho,pm1,pm2p5\n");
  #endif

  float voltage, lux;
  float pm10, pm1, pm2p5, hum_sen5x, temp_sen5x, VOC_sen5x, NOX;
  float temp_bmp280, press, humidity;
  float temp_sfa, hum_sfa, hcho, voc_sgp;
  float bme680_t, bme680_p, bme680_h, bme680_voc;

  int loop=0;
  while (1) {

    sleep_ms(CAPTURE_EVERY_MS);
    remove_CMA(&bmp280_data, &temp_bmp280, &humidity, &press, NULL);

    #ifdef UNIT_INSIDE
    remove_CMA(&sfa30_sgp40_data, &temp_sfa, &hum_sfa, &hcho, &voc_sgp);
    remove_CMA(&sen5x_data_1, &pm1, &pm2p5, &hum_sen5x, NULL);
    remove_CMA(&sen5x_data_2, &temp_sen5x, &VOC_sen5x, &NOX, NULL);
    remove_CMA(&ads1115_data, &voltage, NULL, NULL, NULL);
    remove_CMA(&bme680_dat1, &bme680_t, &bme680_p, &bme680_h, NULL);
    remove_CMA(&bme680_dat2, &bme680_voc, NULL, NULL, NULL);
    printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
           temp_sen5x,
           temp_bmp280,
           temp_sfa,
           bme680_t,
           hum_sen5x,
           hum_sfa,
           bme680_h,
           voltage,
           VOC_sen5x,
           voc_sgp,
           bme680_voc,
           NOX,
           press,
           bme680_p,
           hcho,
           pm1,
           pm2p5);
    float real_temp = mix_sensors({ temp_sen5x, temp_bmp280, bme680_t });
    float real_VOC = mix_sensors({ VOC_sen5x, voc_sgp, bme680_voc });
    float real_hum = mix_sensors({ hum_sen5x, bme680_h });
    float real_pressure = mix_sensors({ press, bme680_p });
    display_data(real_temp, real_hum, voltage, real_VOC, NOX, hcho, pm1);
    record_to_db(real_temp,real_VOC, voltage, real_hum, real_pressure, NOX, hcho, pm1);
    #endif
    #ifdef UNIT_OUTSIDE
    remove_CMA(&light_data, &lux);
    remove_CMA(&ppm_data, &pm1, &pm2p5, &pm10);
    printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
           temp_bmp280,
           humidity,
           press,
           lux,
           pm1,
           pm2p5,
           pm10,
           );
    float real_temp = temp_bmp280;
    float real_hum = humidity;
    float real_pressure = press;
    //record_to_db(real_temp,real_VOC, voltage, real_hum, real_pressure, NOX, hcho, pm1);

    #endif



    if(loop++ > SAVE_EVERY_LOOP)
    {
      flushDB();
      loop=0;
    }
  }
  /*
  std::stringstream sql_transaction_string;
  //float temp, humidity, press, lux, ppm10, ppm25, ppm01;
  int commit=0;
  while (1)
  {

          fflush(NULL);
          commit++;
          if(false)
          //if(commit>=COMMIT_TO_ONLINE)
          {
                  commit=0;
                  try
                  {
                          std::string result_string = "";
                          ret = sqlite3_exec(DB, "select * from sensors;", &per_row_callback, &result_string,
  &messageError); if (ret != SQLITE_OK) { printf("Error REMOVING from Table! %s\n",messageError);
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