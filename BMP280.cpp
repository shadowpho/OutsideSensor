#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#include "BMP280.h"
#include "i2c_helper.h"

#define INTERVAL_MS 1000000


#define READ_BMP280(register_address, recv_buff,num_of_bytes ) assert(0==communicate_I2C(BMP_ADDR,false,register_address,(uint8_t*) recv_buff, num_of_bytes))
#define WRITE_BMP280(register_address, recv_buff,num_of_bytes ) assert(0==communicate_I2C(BMP_ADDR,true,register_address,(uint8_t*) recv_buff, num_of_bytes))


#define BMP280_CHIP_ID                               0xD0
#define BMP280_RESET                                 0xE0
#define BMP280_STATUS                                0xF3
#define BMP280_CTRL_MEAS                             0xF4
#define BMP280_CONFIG                                0xF5
#define BMP280_PRES_MSB                              0xF7
#define BMP280_PRES_LSB                              0xF8
#define BMP280_PRES_XLSB                             0xF9
#define BMP280_TEMP_MSB                              0xFA
#define BMP280_TEMP_LSB                              0xFB
#define BMP280_TEMP_XLSB                             0xFC

#define BMP280_COMP                                  0x88

//reset and block!
int setup_BMP280()
{
    uint8_t buff[2];
    READ_BMP280(BMP280_CHIP_ID,buff,1);
    if(buff[0] != 0x58)
    {
        printf("Wrong device ID for BMP280! %i\n",buff[0]);
        return -1;
    }
    buff[1] = 0xB6 ; //SOFT RESET 
    WRITE_BMP280(BMP280_RESET,buff,1);


    return 0;
}

//blocking read!
int read_from_BMP280(float* temp, float *pressure)
{

    return 0;
}