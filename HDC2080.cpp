#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#include "HDC2080.h"
#include "i2c_helper.h"

#define MFCT_ID 0x5449
#define DEVICE_ID 0x07D0

//device address
#define HDC2080_ADDRESS 0x40

//registers addresses
#define HDC2080_TEMPERATURE_REGISTER (0x00)
#define HDC2080_HUMIDITY_REGISTER (0x02)
#define HDC2080_DRDY_REGISTER (0x04)
#define HDC2080_RESET_REGISTER (0x0E)
#define HDC2080_CONFIG_REGISTER (0x0F)
#define HDC2080_MANUFACTURERID_REGISTER (0xFC)
#define HDC2080_DEVICEID_REGISTER (0xFE)

//bit configuration we actually care about
#define HDC2080_RESET_RESET_BIT (0x80)
#define HDC2080_RESET_HEATER_ENABLE (0x8)
#define HDC2080_CONFIG_GO (0x1)

#define READ_HDC2080(register_address, recv_buff, num_of_bytes) assert(0 == communicate_I2C(HDC2080_ADDRESS, false, register_address, (uint8_t *)recv_buff, num_of_bytes))
#define WRITE_HDC2080(register_address, recv_buff, num_of_bytes) assert(0 == communicate_I2C(HDC2080_ADDRESS, true, register_address, (uint8_t *)recv_buff, num_of_bytes))

//reset and block!
int setup_hdc2080()
{
	//CHECK THAT WE ARE TALKING TO RIGHT DEVICE
	uint16_t response;
	READ_HDC2080(HDC2080_MANUFACTURERID_REGISTER, &response, 2);
	if (response != MFCT_ID)
	{
		printf("Wrong MFCT ID. Wrong device?\n");
		return -4;
	}

	READ_HDC2080(HDC2080_DEVICEID_REGISTER, &response, 2);
	if (response != DEVICE_ID)
	{
		printf("Wrong device ID. Wrong device?\n");
		return -4;
	}
	response = HDC2080_RESET_RESET_BIT; //set high bit enable to reset.
	WRITE_HDC2080(HDC2080_RESET_REGISTER, &response, 1);
	return 0;
}

//blocking read!
int read_from_hdc2080(float *temp, float *humid)
{
	uint8_t buff[2] = {};
	buff[0] = HDC2080_CONFIG_GO;
	WRITE_HDC2080(HDC2080_CONFIG_REGISTER, buff, 1);

	//sleep for 20ms to let it finish measuring
	sleep_ms(20);

	READ_HDC2080(HDC2080_DRDY_REGISTER, buff, 1);
	if ((buff[0] & (1 << 7)) != (1 << 7))
	{
		printf("Reading NOT ready! Reg:%i\n", buff[0]);
		return -1;
	}
	uint16_t temp_raw, humidity_raw;

	READ_HDC2080(HDC2080_TEMPERATURE_REGISTER, &temp_raw, 2);
	READ_HDC2080(HDC2080_HUMIDITY_REGISTER, &humidity_raw, 2);

	*temp = (temp_raw / 65536.0) * 165.0 - 40.62;
	*humid = (humidity_raw / 65536.0) * 100.0;
	return 0;
}