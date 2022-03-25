#include "SEN5x.h"
#include "i2c_helper.h"

#define SEN5x_ADDR 0x69



int SEN5x_start()
{
    uint8_t buff[2];
    buff[0] = 0x04;
    //0xD304 reset, 100ms
    int rslt =  communicate_I2C(SEN5x_ADDR, true, 0xD3, buff, 1);
    if(rslt!=0)
    {
        printf("Failed to reset SEN5x %i\n",rslt); return rslt;
    }
    sleep_ms(100);

    //0x0021 start measurment 50ms
    buff[0] = 0x21;
    rslt =  communicate_I2C(SEN5x_ADDR, true, 0x00, buff, 1);
    if(rslt!=0)
    {
        printf("Failed to start measument with SEN5x %i\n",rslt); return rslt;
    }
    return 0;
}

//0x03C4  read values 30ms
int SEN5x_read(float* pm1, float *pm2p5, float* hum, float* temp, float* VOC, float* NOX)
{
    uint8_t buff[24];
    buff[0] = 0xC4;
    int rslt= communicate_I2C(SEN5x_ADDR,true,0x03,buff,1);
    if(rslt!=0)
    {
        printf("Failed to send GET measument with SEN5x %i\n",rslt); return rslt;
    }
    sleep_ms(50);
    rslt= read_I2C(SEN5x_ADDR,buff,24);
    if(rslt!=0)
    {
        printf("Failed to retr measument with SEN5x %i\n",rslt); return rslt;
    }
    *pm1 =  ((uint16_t)buff[0] << 8 | buff[1]) / 10.0;
    *pm2p5 =  ((uint16_t)buff[3] << 8 | buff[4]) / 10.0;
    *hum =  ((uint16_t)buff[12] << 8 | buff[13]) / 100.0;
    *temp =  ((uint16_t)buff[15] << 8 | buff[16]) / 200.0;
    *VOC =  ((uint16_t)buff[18] << 8 | buff[19]) / 10.0;
    *NOX =  ((uint16_t)buff[21] << 8 | buff[22]) / 10.0;
    return 0;
}

