#include <stdbool.h>
#include <stdint.h>

// parsed measurement data
typedef struct {
    uint16_t concPM1_0_CF1;
    uint16_t concPM2_5_CF1;
    uint16_t concPM10_0_CF1;
    uint16_t concPM1_0_amb;
    uint16_t concPM2_5_amb;
    uint16_t concPM10_0_amb;
    uint16_t rawGt0_3um;
    uint16_t rawGt0_5um;
    uint16_t rawGt1_0um;
    uint16_t rawGt2_5um;
    uint16_t rawGt5_0um;
    uint16_t rawGt10_0um;
    uint8_t version;
    uint8_t errorCode;
} pms_meas_t;

void setup_PMS7003(void);
void read_from_PMS(uint16_t *pm10, uint16_t *pm25,uint16_t *pm1);


