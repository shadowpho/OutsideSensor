#pragma once


#include "../i2c_helper.h"

//(*bme68x_read_fptr_t)
signed char user_i2c_read(unsigned char reg_addr, unsigned char *reg_data, unsigned int length,void *intf_ptr)
{
    return communicate_I2C(BME68X_I2C_ADDR_HIGH,false,reg_addr,reg_data,length);
}

signed char user_i2c_write(unsigned char reg_addr, const unsigned char *reg_data, unsigned int length,void *intf_ptr)
{
    return communicate_I2C(BME68X_I2C_ADDR_HIGH,true,reg_addr,(unsigned char*)reg_data,length);
}

void user_delay_us(uint32_t period, void *intf_ptr)
{
    sleep_us(period);
}
