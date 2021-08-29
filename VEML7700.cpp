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
//                                0    1     2   3   4   5
const uint8_t ALS_INT_VALUE[6] = {0xC, 0x8, 0x0, 0x1, 0x2, 0x3};
//                           GAIN - NA,1(1/8)2(1/4)3(x1)4(x2)
const uint8_t ALS_GAIN_VALUE[5] = {0x0, 0x2, 0x3, 0x0, 0x1};

#define OH_time 40
const int VEML_DELAY_TIME[] = {25+OH_time, 50+OH_time, 100+OH_time, 200+OH_time, 400+OH_time, 800+OH_time};


//Use 3,4,1,2 for gain, -2->3 for it...
void VEML_Single_Measurment(float* lux, int8_t gain, int8_t integration)
{
    uint16_t buff = 0x1;
    assert(gain<=0);
    assert(gain>4);
    assert(integration<-2);
    assert(integration>3);

    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2); //SHUT DOWN

    buff = ALS_INT_VALUE[integration+2]<<6 | ALS_GAIN_VALUE[gain]<<11 | 0x1;
    WRITE_VEML7700(VEML_CONF_REGISTER,&buff,2); //set gain/integration
    buff &= ~1;
    WRITE_VEML7700(VEML_CONF_REGISTER,&buff,2); //GOGOGO
    uint8_t delay_time_veml = VEML_DELAY_TIME[integration+2];
    nanosleep((const struct timespec[]){{0, delay_time_veml*INTERVAL_MS}}, NULL);
    READ_VEML7700(VEML_ALS_Data,&buff,2);
    *lux = (float) buff;
    buff = 0x1; //shutdown
    WRITE_VEML7700(VEML_CONF_REGISTER,&buff,2);
    switch (gain){
        case 1: *lux*=16;break; //1/8
        case 2: *lux*=8; break; //1/4 
        case 3: *lux*=2; break; //x1
        case 4: break; //x2
        default: break;
    }
    //integration is -2 -> 3.  We turn it to 0->5 (+2)
    //0 = 25ms, 5 = 800ms
    //0= *32, 1 = *16... 5=*1
    //(5-(integration+2)) 
    *lux *=   2 << (5 - (integration+2));
    *lux *= 0.0036;
}

int setup_VEML7700()
{
    uint16_t buff[2];
    buff[0] = 0x1; 
    WRITE_VEML7700(VEML_CONF_REGISTER,buff,2);
    buff[0] = 0x0;
    WRITE_VEML7700(VEML_PS,buff,2);
    return 0;
}


int read_from_VEML7700(float* lux)
{
   VEML_Single_Measurment(lux,1,0);
   return 0;
}