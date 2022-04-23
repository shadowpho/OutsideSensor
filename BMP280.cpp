#include <cassert>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "BMP280.h"
#include "i2c_helper.h"

#define READ_BMP280(register_address, recv_buff, num_of_bytes) \
  assert(0 == communicate_I2C(BMP_ADDR, false, register_address, (uint8_t*)recv_buff, num_of_bytes))
#define WRITE_BMP280(register_address, recv_buff, num_of_bytes) \
  assert(0 == communicate_I2C(BMP_ADDR, true, register_address, (uint8_t*)recv_buff, num_of_bytes))

#define BMP280_CHIP_ID 0xD0
#define BMP280_RESET 0xE0
#define BMP280_STATUS 0xF3
#define BMP280_CTRL_MEAS 0xF4
#define BMP280_CONFIG 0xF5
#define BMP280_PRES_MSB 0xF7
#define BMP280_PRES_LSB 0xF8
#define BMP280_PRES_XLSB 0xF9
#define BMP280_TEMP_MSB 0xFA
#define BMP280_TEMP_LSB 0xFB
#define BMP280_TEMP_XLSB 0xFC

#define BMP280_COMP 0x88

struct
{
  uint16_t dig_t1; // 0x88 and 0x89
  int16_t dig_t2;
  int16_t dig_t3;
  uint16_t dig_p1;
  int16_t dig_p2;
  int16_t dig_p3;
  int16_t dig_p4;
  int16_t dig_p5;
  int16_t dig_p6;
  int16_t dig_p7;
  int16_t dig_p8;
  int16_t dig_p9;
} compensation;

// reset and block!
int setup_BMP280()
{
  uint8_t buff[2];
  READ_BMP280(BMP280_CHIP_ID, buff, 1);
  if (buff[0] != 0x58) {
    printf("Wrong device ID for BMP280! %i\n", buff[0]);
    return -1;
  }
  READ_BMP280(BMP280_COMP, &compensation, sizeof(compensation));

  buff[1] = 0xB6; // SOFT RESET
  WRITE_BMP280(BMP280_RESET, buff, 1);
  sleep_ms(5);
  buff[0] = 0; // IIR off, no wait,
  WRITE_BMP280(BMP280_CONFIG, buff, 1);

  return 0;
}

// blocking read!
int read_from_BMP280(float* temp, float* return_pressure)
{
  uint8_t buff[4];
  uint32_t uncomp_press;
  int32_t uncomp_temp;
  int32_t temperature;
  uint32_t pressure;
  int32_t t_fine;

  assert(temp != nullptr);
  assert(return_pressure != nullptr);

  buff[0] = 7 << 5 | 7 << 2 | 1; //  MAX OVERSAMPLING| forced mode
  WRITE_BMP280(BMP280_CTRL_MEAS, buff, 1);

  sleep_ms(200); // 70 ms too soon, so let's wait 140 mS

  READ_BMP280(BMP280_CTRL_MEAS, buff, 1);
  if (buff[0] != (7 << 5 | 7 << 2)) {
    printf("BMP280 Error! still running! %i\n", buff[0]);

    return -1;
  }

  READ_BMP280(BMP280_PRES_MSB, buff, 3);
  uncomp_press = (uint32_t)buff[0] << 12 | (uint32_t)buff[1] << 4 | (uint32_t)buff[2] >> 4;
  READ_BMP280(BMP280_TEMP_MSB, buff, 3);
  uncomp_temp = (int32_t)buff[0] << 12 | (int32_t)buff[1] << 4 | (int32_t)buff[2] >> 4;
  {
    int32_t var1, var2;
    var1 = ((((uncomp_temp / 8) - ((int32_t)compensation.dig_t1 * 2))) * ((int32_t)compensation.dig_t2)) / 2048;
    var2 = (((((uncomp_temp / 16) - ((int32_t)compensation.dig_t1)) *
              ((uncomp_temp / 16) - ((int32_t)compensation.dig_t1))) /
             4096) *
            ((int32_t)compensation.dig_t3)) /
           16384;

    t_fine = var1 + var2;

    temperature = (t_fine * 5 + 128) / 256;
  }
  {
    int64_t var1, var2, p;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)compensation.dig_p6;
    var2 = var2 + ((var1 * (int64_t)compensation.dig_p5) * 131072);
    var2 = var2 + (((int64_t)compensation.dig_p4) * (int64_t)34359738368);
    var1 = ((var1 * var1 * (int64_t)compensation.dig_p3) / 256) + ((var1 * (int64_t)compensation.dig_p2) * 4096);
    var1 = (((((int64_t)1) * (int64_t)140737488355328) + var1)) * ((int64_t)compensation.dig_p1) / (int64_t)8589934592;

    if (var1 != 0) {
      p = 1048576 - uncomp_press;
      p = (((p * 2147483648) - var2) * 3125) / var1;
      var1 = (((int64_t)compensation.dig_p9) * (p / 8192) * (p / 8192)) / 33554432;
      var2 = (((int64_t)compensation.dig_p8) * p) / 524288;

      p = ((p + var1 + var2) / 256) + (((int64_t)compensation.dig_p7) * 16);
      pressure = (uint32_t)p;
    }
  }
  *return_pressure = (float)pressure / 256.0;
  *temp = ((float)temperature) / 100.0;
  return 0;
}