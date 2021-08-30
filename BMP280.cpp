#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#include "BMP280.h"
#include "i2c_helper.h"

#define INTERVAL_MS 1000000

#define READ_BMP280(register_address, recv_buff, num_of_bytes) assert(0 == communicate_I2C(BMP_ADDR, false, register_address, (uint8_t *)recv_buff, num_of_bytes))
#define WRITE_BMP280(register_address, recv_buff, num_of_bytes) assert(0 == communicate_I2C(BMP_ADDR, true, register_address, (uint8_t *)recv_buff, num_of_bytes))

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
    uint16_t dig_T1; //0x88 and 0x89
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
} compensation;

//reset and block!
int setup_BMP280()
{
    uint8_t buff[2];
    READ_BMP280(BMP280_CHIP_ID, buff, 1);
    if (buff[0] != 0x58)
    {
        printf("Wrong device ID for BMP280! %i\n", buff[0]);
        return -1;
    }
    READ_BMP280(BMP280_COMP, &compensation, sizeof(compensation));

    buff[1] = 0xB6; //SOFT RESET
    WRITE_BMP280(BMP280_RESET, buff, 1);
    return 0;
}

//blocking read!
int read_from_BMP280(float *temp, float *pressure)
{
    uint8_t buff[3];

    assert(temp!=nullptr);
    assert(pressure!=nullptr);

    WRITE_BMP280(BMP280_CTRL_MEAS,buff,1);
    WRITE_BMP280(BMP280_CONFIG,buff,1);

    READ_BMP280(BMP280_STATUS,buff,1);

    READ_BMP280(BMP280_PRES_MSB,buff,3);

    READ_BMP280(BMP280_TEMP_MSB,buff,3);
    return 0;
}