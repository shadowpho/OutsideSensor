#include "SGP40.h"
#include "sensirion_gas_index_algorithm.h"
#include "i2c_helper.h"

#define CRC8_INIT 0xFF
#define CRC8_POLYNOMIAL 0x31

#define SGP40_ADDR 0x59

GasIndexAlgorithmParams voc_params;

uint8_t sensirion_i2c_generate_crc(const uint8_t* data, uint16_t count) {
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}


int SGP40_start()
{
    uint8_t buff[4];
    GasIndexAlgorithm_init(&voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);
    buff[0] = 0x0E;
    int rslt = communicate_I2C(SGP40_ADDR, true, 0x28 , buff, 1);
    if(rslt!=0)
    {
        printf("Error writing self test command for SGP40! %i\n",rslt); return rslt;
    }
    sleep_ms(400);
    rslt = read_I2C(SGP40_ADDR, buff,3);
    if(rslt!=0)
    {
        printf("Error reading self test results for SGP40! %i\n",rslt); return rslt;
    }
    if(buff[0] != 0xD4)
    {
        printf("Error self test results for SGP40! %i\n",buff[0]); return -9;
    }
    return 0;
}

//call this 1x a second
int SGP40_read(float temp, float humidity, int32_t* VOC)
{
    uint16_t sraw_voc = 0;
    uint8_t buff[8];
    //0x26 0x0F, send 6, get 3
    //RH% * 65535/100 2 bytes + CRC
    // (T + 45) * 65535/175 + CRC
    //return = 2 bytes + CRC
    //30 ms
    buff[0] = 0x0F;
    buff[1] =  ((uint16_t)(humidity * 65535.0/100)) >> 8;
    buff[2] =  (uint8_t) ((uint16_t)(humidity * 65535.0/100));
    buff[3] = sensirion_i2c_generate_crc((const uint8_t*)buff+1, 2);
    buff[4] = ((uint16_t)((temp + 45)*65535/175)) >> 8;
    buff[5] = (uint8_t) ((uint16_t)((temp + 45)*65535/175));
    buff[6] = sensirion_i2c_generate_crc((const uint8_t*)buff+4, 2);
    int rslt = communicate_I2C(SGP40_ADDR, true, 0x26 , buff, 7);
    if(rslt!=0)
    {
        printf("Error writing run command for SGP40! %i\n",rslt); return rslt;
    }
    sleep_ms(50);
    rslt = read_I2C(SGP40_ADDR, buff,3);
    if(rslt!=0)
    {
        printf("Error reading results for SGP40! %i\n",rslt); return rslt;
    }
    sraw_voc = (uint16_t)buff[0] << 8 | (uint16_t)buff[1];

    GasIndexAlgorithm_process(&voc_params, sraw_voc, VOC);
    return 0;
}