#include "ADS1115.h"
#include "i2c_helper.h"

#define ADS1115_ADDR 0x48

char buff[4];


int ads1115_start()
{
    buff[0]=0x41;
    buff[1]=0x03;
    return communicate_I2C(ADS1115_ADDR,true, 0x1,buff,2);   
}

int ads1115_read(float* voltage)
{
     // x100 yyy1 0000 0011  DR=8SPS, disable comp
    //x=go, yyy=PGA
    const uint8_t default_mask = 0x4103; //0100 0001 0000 0011
    uint8_t PGA; //0.256v
    int16_t result_voltage = 0;

    for(PGA=5;PGA>0;PGA--)
    {
        uint8_t config_rgr = default_mask | (PGA <<9) | (1<<15);
        int rslt = communicate_I2C(ADS1115_ADDR,true,0x1,buff,2);
        if(rslt!=0) {printf("Failed to write config to ADS1115!!\n");return rslt;}
        sleep_ms(1000/8 * 2); //conv time is 1/8 of second, *2 to be safe

        rslt = communicate_I2C(ADS1115_ADDR,false,0x0,buff,2);
        if(rslt!=0) {printf("Failed to read results from ADS1115!!\n");return rslt;}
        result_voltage = (int16_t) ((uint16_t)buff[0] << 8 | buff[1]); 
        if(result_voltage < 0xF8) break; 

    }
    *result = ((double)result_voltage / 0x7FFF);
    switch (PGA){
        case 0: *result*=6.144; break;
        case 1: *result*=4.096; break;
        case 2: *result*=2.048; break;
        case 3: *result*=1.024; break;
        case 4: *result*=0.512; break;
        case 5: *result*=0.256; break;
        default: printf("PGA out of bounds!!!%i\n",PGA); return -1;
    } 
    return 0;
}