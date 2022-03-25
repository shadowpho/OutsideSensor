#include "SFA30x.h"
#include "i2c_helper.h"

#define SFA30x_ADDR 0x5D

int sfa3x_start()
{
    uint8_t buf[2];
    buf[0] = 0x04; 
    //SELF RESET
    int t_err = communicate_I2C(SFA30x_ADDR,true,0xD3, buf,1);
    if(t_err != 0)
    {
        printf("Failed to reset SFA3x! %i\n", t_err);
        return -1;
    }
    sleep_ms(200);

    buf[0] = 0x06; //START
    t_err = communicate_I2C(SFA30x_ADDR,true,0x00, buf,1);
    if(t_err != 0)
    {
        printf("Failed to start SFA3x! %i\n", t_err);
        return -1;
    }
    return 0;
}

int sfa3x_stop()
{
    uint8_t buf[2];
    buf[0] = 0x04;
    int t_err = communicate_I2C(SFA30x_ADDR,true,0x01, buf,1);
    if(t_err != 0)
    {
        printf("Failed to stop SFA3x! %i\n", t_err);
        return -1;
    }
    return 0;
}

int sfa3x_read(float* hcho, float* humidity,float* temperature)
{
    uint8_t wbuf[4];
    uint8_t rbuf[8];
    wbuf[0] = 0x03;
    wbuf[1] = 0x27;
    int t_err = write_read_I2C(SFA30x_ADDR, wbuf,2,rbuf,8);

    if(t_err != 0)
    {
        printf("Could not write to read out SFA30x! %i\n", t_err);
        return -1;
    }

    int16_t temp_value = ((uint16_t)rbuf[0] << 8 | rbuf[1]); 
    *hcho = temp_value / 5.0; 
    temp_value = ((uint16_t)rbuf[3] << 8 | rbuf[4]); 
    *humidity = temp_value / 100.0;
    temp_value = ((uint16_t)rbuf[6] << 8 | rbuf[7]); 
    *temperature = temp_value / 200.0;

    return 0;
}
