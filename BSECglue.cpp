#include "BSECglue.h"
#include <cassert>

#include "BME680/bme68x_adapter.h"

#include "BSEC/bsec_interface.h"
#include "BSEC/bsec_serialized_configurations_selectivity.h"
#include "BSEC/bsec_serialized_configurations_iaq.h"

#include <vector>
#include <chrono>

int BSEC_BME_selftest()
{
    struct bme68x_dev gas_sensor;
    gas_sensor.intf = BME68X_I2C_INTF;
    gas_sensor.read = user_i2c_read;
    gas_sensor.write = user_i2c_write;
    gas_sensor.delay_us = user_delay_us;
    gas_sensor.amb_temp = 20;
    int rslt = bme68x_selftest_check(&gas_sensor);
    return rslt;
}

int BSEC_BME_init()
{
    int ret = 0;
    ret = bsec_init();
    if (ret != 0)
        return ret;
    struct bme68x_dev gas_sensor;
    gas_sensor.intf = BME68X_I2C_INTF;
    gas_sensor.read = user_i2c_read;
    gas_sensor.write = user_i2c_write;
    gas_sensor.delay_us = user_delay_us;
    gas_sensor.amb_temp = 20;
    ret = bme68x_init(&gas_sensor);
    if (ret != 0)
        return ret;
    std::vector<uint8_t> work_buffer(BSEC_MAX_PROPERTY_BLOB_SIZE);

    ret = bsec_set_configuration(bsec_config_iaq, BSEC_MAX_PROPERTY_BLOB_SIZE, work_buffer.data(), work_buffer.size());
    if (ret != 0)
        return ret;

    bsec_sensor_configuration_t requested_virtual_sensors[4];
    uint8_t n_requested_virtual_sensors = 4;

    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_STATIC_IAQ;
    requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_HIGH_PERFORMANCE;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
    requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_HIGH_PERFORMANCE;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
    requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_HIGH_PERFORMANCE;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_HIGH_PERFORMANCE;

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    ret = bsec_update_subscription(requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);
    if (ret != 0)
        return ret;
    return 0;
}

int BSEC_BME_loop()
{
    using namespace std::chrono;
    int64_t now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    if(now < 0) return 0; //XXX

    bsec_bme_settings_t bme680_settings;
    bsec_sensor_control(now,&bme680_settings);
    bsec_input_t input[3];
    uint8_t n_input = 3;
    bsec_output_t output[4];
    uint8_t n_output = 4;

    bsec_library_return_t status;
    float R=0,T=0,rH=0;
    // Populate the input structs, assuming the we have timestamp (ts),
    // gas sensor resistance (R), temperature (T), and humidity (rH) available
    // as input variables
    input[0].sensor_id = BSEC_INPUT_GASRESISTOR;
    input[0].signal = R;
    input[0].time_stamp = now;
    input[1].sensor_id = BSEC_INPUT_TEMPERATURE;
    input[1].signal = T;
    input[1].time_stamp = now;
    input[2].sensor_id = BSEC_INPUT_HUMIDITY;
    input[2].signal = rH;
    input[2].time_stamp = now;
    status = bsec_do_steps(input, n_input, output, &n_output);
    assert(status == BSEC_OK);
    for (int i = 0; i < n_output; i++)
    {
        switch (output[i].sensor_id)
        {
        case BSEC_OUTPUT_IAQ:
            // Retrieve the IAQ results from output[i].signal
            // and do something with the data
            break;
            // case BSEC_OUTPUT_AMBIENT_TEMPERATURE:
            //  Retrieve the ambient temperature results from output[i].signal
            //  and do something with the data
            break;
        }
    }
    return 0;
}