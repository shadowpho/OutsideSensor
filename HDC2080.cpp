#include <stdio.h>
#include <time.h>
#include "HDC2080.h"
#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//
		

#include <errno.h>
#include <string.h>

#define MFCT_ID 0x5449
#define DEVICE_ID 0x07D0

static int file_i2c_handle =0;


//device address
#define HDC2080_ADDRESS 0x40

//registers addresses
#define HDC2080_TEMPERATURE_REGISTER           (0x00)
#define HDC2080_HUMIDITY_REGISTER              (0x02)
#define HDC2080_RESET_REGISTER                (0x0E)
#define HDC2080_CONFIG_REGISTER        (0x0F)
#define HDC2080_MANUFACTURERID_REGISTER        (0xFC)
#define HDC2080_DEVICEID_REGISTER         (0xFE)

//bit configuration we actually care about
#define HDC2080_RESET_RESET_BIT              (0x80)
#define HDC2080_RESET_HEATER_ENABLE           (0x8)
#define HDC2080_CONFIG_GO        (0x1)

#define NANO_MS_MULTIPLIER  1000000L;                // 1 millisecond = 1,000,000 Nanoseconds
const int64_t INTERVAL_MS = NANO_MS_MULTIPLIER;



//reset and block!
int setup_hdc2080()
{

    return 0;
}

//blocking read!
int read_from_hdc2080(float* temp, float *humid)
{

    return 0;
}