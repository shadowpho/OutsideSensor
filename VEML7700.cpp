#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#include "VEML7700.h"
#include "i2c_helper.h"

#define INTERVAL_MS 1000000

#define VEML_CONF_REGISTER 0x0
#define VEML_PS            0x1
#define VEML_ALS_Data      0x4
#define VEML_White_Data    0x5
#define VEML_INT           0x6


#define READ_VEML7700(register_address, recv_buff,num_of_bytes ) assert(0==communicate_I2C(VEML7700_ADDR,false,register_address,(uint8_t*) recv_buff, num_of_bytes))
#define WRITE_VEML7700(register_address, recv_buff,num_of_bytes ) assert(0==communicate_I2C(VEML7700_ADDR,true,register_address,(uint8_t*) recv_buff, num_of_bytes))

#define sleep_ms(num_of_ms) nanosleep((const struct timespec[]){{0, num_of_ms*INTERVAL_MS}}, NULL)

/* From DS:
ALS integration time setting IT:
1100 = 25 ms → -2
1000 = 50 ms → -1
0000 = 100 ms → 0
0001 = 200 ms → 1
0010 = 400 ms → 2
0011 = 800 ms → 3
Gain selection G:
00 = ALS gain x 1 → 3
01 = ALS gain x 2 → 4
10 = ALS gain x (1/8) → 1
11 = ALS gain x (1/4) → 2
*/
const uint8_t ALS_INT_VALUE[6] = {0xC, 0x8, 0x0, 0x1, 0x2, 0x3};
const uint8_t ALS_GAIN_VALUE[4] = {0x0, 0x1, 0x2, 0x3};


//also kills other settings!!!!
int enable_VEML7700(bool to_enable)
{
    uint16_t rgst_read; 
    if(to_enable)
        rgst_read = 0x0; //enable 
    else
        rgst_read = 0x1; //shut down
    WRITE_VEML7700(VEML_CONF_REGISTER,&rgst_read,2);
    return 0;
}
//Use 3,4,1,2 for gain, -2->3 for it...
void VEML_Single_Measurment(float* lux, uint8_t gain, uint8_t integration)
{
    uint16_t buff = 0x0;
    enable_VEML7700(false);
    buff = 

    enable_VEML7700(true);
}

int setup_VEML7700()
{
    uint16_t buff[2];
    buff[0] = 0x1; 
    WRITE_VEML7700(VEML_CONF_REGISTER,buff,2);
    buff[0] = 0x0;
    WRITE_VEML7700(VEML_PS,buff,2);
    return 1;
}


int read_from_VEML7700(float* temp, float *pressure)
{
    enable_VEML7700(false); //turn it off.
    //configure for first 
    sleep_ms(5);
    //enable and wait
    enable_VEML7700(true);
    sleep_ms(5);

    
    return 1;
}