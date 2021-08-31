#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#include "VEML7700.h"
#include "i2c_helper.h"

#define VEML_CONF_REGISTER 0x0
#define VEML_PS 0x1
#define VEML_ALS_Data 0x4
#define VEML_White_Data 0x5
#define VEML_INT 0x6

#define READ_VEML7700(register_address, recv_buff, num_of_bytes) assert(0 == communicate_I2C(VEML7700_ADDR, false, register_address, (uint8_t *)recv_buff, num_of_bytes))
#define WRITE_VEML7700(register_address, recv_buff, num_of_bytes) assert(0 == communicate_I2C(VEML7700_ADDR, true, register_address, (uint8_t *)recv_buff, num_of_bytes))

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
enum VEML_IT_TIME{
VEML_25MS = -2,
VEML_50MS = -1,
VEML_100MS = 0,
VEML_200MS = 1,
VEML_400MS = 2,
VEML_800MS = 3
};

//                                0    1     2   3   4   5
const uint8_t ALS_INT_VALUE[6] = {0xC, 0x8, 0x0, 0x1, 0x2, 0x3};
//                           GAIN - NA,1(1/8)2(1/4)3(x1)4(x2)
const uint8_t ALS_GAIN_VALUE[5] = {0x0, 0x2, 0x3, 0x0, 0x1};

#define OH_time 60
#define OH_MULT 1.2
const int VEML_DELAY_TIME[] = {60,110, 200,350, 700,1200 };

//Use 3,4,1,2 for gain, -2->3 for IT
//Returns raw count
unsigned int VEML_Single_Measurment(float *lux, int8_t gain, int8_t integration)
{
    uint16_t buff = 0x1;
    uint16_t als_count = 0;
    assert(gain > 0);
    assert(gain <= 4);
    assert(integration >= -2);
    assert(integration <= 3);

    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2); //SHUT DOWN

    buff = ALS_INT_VALUE[integration + 2] << 6 | ALS_GAIN_VALUE[gain] << 11 | 0x1;
    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2); //set gain/integration
    buff &= ~1;
    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2); //GOGOGO
    int delay_time_veml = VEML_DELAY_TIME[integration + 2];
    sleep_ms(delay_time_veml);
    READ_VEML7700(VEML_ALS_Data, &als_count, 2);
    printf("Lux!G:%i,I:%i,R:%i\n", gain, integration, als_count);
    *lux = (float)als_count;
    buff = 0x1; //shutdown
    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2);
    switch (gain)
    {
    case 1:
        *lux *= 16;
        break; //1/8
    case 2:
        *lux *= 8;
        break; //1/4
    case 3:
        *lux *= 2;
        break; //x1
    case 4:
        break; //x2
    default:
        break;
    }
    //integration is -2 -> 3.  We turn it to 0->5 (+2)
    //0 = 25ms, 5 = 800ms
    //0= *32, 1 = *16... 5=*1
    //(5-(integration+2))
    *lux *= 1 << (5 - (integration + 2));
    *lux *= 0.0036;
    return als_count;
}

int setup_VEML7700()
{
    uint16_t buff[2];
    buff[0] = 0x1;
    WRITE_VEML7700(VEML_CONF_REGISTER, buff, 2);
    buff[0] = 0x0;
    WRITE_VEML7700(VEML_PS, buff, 2);
    return 0;
}

int read_from_VEML7700(float *lux)
{
    int gain=1;
    int integration=0;
    int ret_count = 0;
    int loop_limit = 50; 

    do{
        ret_count = VEML_Single_Measurment(lux, gain, integration);
        if(ret_count >100)
            break; //nove to second loop
        gain++;
        if(gain>=4)
            {
                integration++;
                gain=4;
            }
        if(integration==4)
            return 0; //done! Probably dark
    }while(loop_limit-- > 0);

    loop_limit = 50; //reset loop_limit
    //Second loop
    do
    {
        if(ret_count< 10000)
            break; //done! between 100 and 10,000
        integration--;
        if(integration==-3) break; //very bright
        ret_count = VEML_Single_Measurment(lux, gain, integration);
    } while (loop_limit-- > 0);
    
    float lux_calc = *lux;
    if(gain==1)
        *lux = lux_calc * (1.0023 + lux_calc * (0.000081488 + lux_calc * (-9.3924e-9 + 6.0135e-13 * lux_calc)));

    return 0;
}