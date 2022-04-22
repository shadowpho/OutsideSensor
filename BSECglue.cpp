#include "BSECglue.h"
#include <cassert>

#include "BME680/bme68x_adapter.h"

#include "BSEC/bsec_interface.h"
#include "BSEC/bsec_serialized_configurations_selectivity.h"
#include "BSEC/bsec_serialized_configurations_iaq.h"

#include <vector>
#include <chrono>

static struct bme68x_dev gas_sensor;
int64_t next_call_ns;

int BSEC_BME_selftest()
{
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
    next_call_ns = 0;
    int ret = 0;
    ret = bsec_init();
    if (ret != 0)
        return ret;
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
    if(now < next_call_ns) return -1; //too early

    int ret=0;
    bsec_bme_settings_t bme680_settings = {0};
    ret = bsec_sensor_control(now,&bme680_settings);
    if(ret!=0) return ret; 
    next_call_ns = bme680_settings.next_call; 
    if (bme680_settings.trigger_measurement!=1) 
        return 0;
    
    struct bme68x_conf new_conf;
    new_conf.os_hum = bme680_settings.humidity_oversampling;
    new_conf.os_temp = bme680_settings.temperature_oversampling; 
    new_conf.os_pres = bme680_settings.pressure_oversampling;
    ret = bme68x_set_conf(&new_conf,&gas_sensor);
    if(ret!=0) return ret; 

    struct bme68x_heatr_conf heater_conf = {0};
    heater_conf.enable = bme680_settings.run_gas;
    heater_conf.heatr_temp= bme680_settings.heater_temperature;
    heater_conf.heatr_dur = bme680_settings.heater_duration;
    heater_conf.heatr_temp_prof = bme680_settings.heater_temperature_profile;
    heater_conf.heatr_dur_prof = bme680_settings.heater_duration_profile;
    heater_conf.profile_len = bme680_settings.heater_profile_len;
    ret = bme68x_set_heatr_conf(bme680_settings.op_mode,&heater_conf,&gas_sensor);
    if(ret!=0) return ret; 
    
    //ret = bme68x_set_op_mode(bme680_settings.op_mode, &gas_sensor);
    //if(ret!=0) return ret; 

    //XXX
    sleep_ms(100);
    //XXX
    if(bme680_settings.process_data == 0 ) return 0;
    struct bme68x_data new_data; 
    uint8_t n_fields;
    ret = bme68x_get_data(bme680_settings.op_mode,&new_data,&n_fields,&gas_sensor);
    if(ret!=0) return 0;
    assert(new_data.status & BME68X_NEW_DATA_MSK);

    bsec_input_t input[3];
    uint8_t n_input = 3;
    bsec_output_t output[5];
    uint8_t n_output = 5;

    bsec_library_return_t status;
    now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    input[0].sensor_id = BSEC_INPUT_GASRESISTOR;
    input[0].signal = new_data.gas_resistance;
    input[0].time_stamp = now;
    input[1].sensor_id = BSEC_INPUT_TEMPERATURE;
    input[1].signal = new_data.temperature;
    input[1].time_stamp = now;
    input[2].sensor_id = BSEC_INPUT_HUMIDITY;
    input[2].signal = new_data.humidity;
    input[2].time_stamp = now;
    status = bsec_do_steps(input, n_input, output, &n_output);
    assert(status == BSEC_OK);
    for (int i = 0; i < n_output; i++)
    {
        printf("%i:%i\n",output[i].sensor_id,output[i].signal);
    }
    return 0;
}