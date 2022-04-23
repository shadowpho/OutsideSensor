#include "BSECglue.h"
#include <cassert>

#include "BME680/bme68x_adapter.h"

#include "BSEC/bsec_interface.h"
#include "BSEC/bsec_serialized_configurations_iaq.h"
#include "BSEC/bsec_serialized_configurations_selectivity.h"

#include <chrono>
#include <vector>

static struct bme68x_dev gas_sensor;
int64_t next_call_ns;

#define CHK_RTN_BSEC(x)                         \
  do {                                          \
    int _rslt = (x);                            \
    if (_rslt != 0) {                           \
      printf("BSEC Error!%s, %d\n", #x, _rslt); \
      return _rslt;                             \
    }                                           \
  } while (0)
#define CHK_RTN_BME(x)                         \
  do {                                         \
    int _rslt = (x);                           \
    if (_rslt != 0) {                          \
      printf("BME Error!%s, %d\n", #x, _rslt); \
      return _rslt;                            \
    }                                          \
  } while (0)

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
  CHK_RTN_BSEC(bsec_init());

  gas_sensor.intf = BME68X_I2C_INTF;
  gas_sensor.read = user_i2c_read;
  gas_sensor.write = user_i2c_write;
  gas_sensor.delay_us = user_delay_us;
  gas_sensor.amb_temp = 22;
  CHK_RTN_BME(bme68x_init(&gas_sensor));

  std::vector<uint8_t> work_buffer(BSEC_MAX_PROPERTY_BLOB_SIZE);

  CHK_RTN_BSEC(
    bsec_set_configuration(bsec_config_iaq, BSEC_MAX_PROPERTY_BLOB_SIZE, work_buffer.data(), work_buffer.size()));

  bsec_sensor_configuration_t requested_virtual_sensors[4];
  uint8_t n_requested_virtual_sensors = 4;

  requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_STATIC_IAQ;
  requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_LP;
  requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
  requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_LP;
  requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
  requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_LP;
  requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
  requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_LP;

  bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
  uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

  CHK_RTN_BSEC(bsec_update_subscription(
    requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings));

  return 0;
}

int BSEC_desired_sleep_us()
{
  using namespace std::chrono;
  int64_t now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  return ((next_call_ns - now) / 1000) + 1;
}
int BSEC_BME_loop(float* temp, float* pressure, float* humidity, float* VOC)
{
  *temp = INVALID_VALUE;
  *pressure = INVALID_VALUE;
  *humidity = INVALID_VALUE;
  *VOC = INVALID_VALUE;
  using namespace std::chrono;
  int64_t now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  if (now < next_call_ns) return -1; // too early

  bsec_bme_settings_t bme680_settings = { 0 };
  CHK_RTN_BSEC(bsec_sensor_control(now, &bme680_settings));

  next_call_ns = bme680_settings.next_call;
  if (bme680_settings.trigger_measurement != 1) return 0;

  struct bme68x_conf new_conf = { 0 };
  new_conf.os_hum = bme680_settings.humidity_oversampling;
  new_conf.os_temp = bme680_settings.temperature_oversampling;
  new_conf.os_pres = bme680_settings.pressure_oversampling;
  CHK_RTN_BME(bme68x_set_conf(&new_conf, &gas_sensor));

  struct bme68x_heatr_conf heater_conf = { 0 };
  heater_conf.enable = bme680_settings.run_gas;
  heater_conf.heatr_temp = bme680_settings.heater_temperature;
  heater_conf.heatr_dur = bme680_settings.heater_duration;
  heater_conf.heatr_temp_prof = bme680_settings.heater_temperature_profile;
  heater_conf.heatr_dur_prof = bme680_settings.heater_duration_profile;
  heater_conf.profile_len = bme680_settings.heater_profile_len;
  heater_conf.shared_heatr_dur = 140 - (bme68x_get_meas_dur(bme680_settings.op_mode, &new_conf, &gas_sensor) / 1000);
  CHK_RTN_BME(bme68x_set_heatr_conf(bme680_settings.op_mode, &heater_conf, &gas_sensor));

  CHK_RTN_BME(bme68x_set_op_mode(bme680_settings.op_mode, &gas_sensor));
  if (bme680_settings.process_data == 0) return 0;
  uint32_t del_period =
    bme68x_get_meas_dur(bme680_settings.op_mode, &new_conf, &gas_sensor) + (heater_conf.shared_heatr_dur * 1000);
  sleep_us(del_period);
  sleep_ms(100);
  struct bme68x_data new_data;
  uint8_t n_fields;
  CHK_RTN_BME(bme68x_get_data(bme680_settings.op_mode, &new_data, &n_fields, &gas_sensor));
  assert(new_data.status & BME68X_NEW_DATA_MSK);

  bsec_input_t input[BSEC_MAX_PHYSICAL_SENSOR];
  uint8_t n_input = 0;
  bsec_output_t output[5];
  uint8_t n_output = 5;

  if (bme680_settings.process_data & BSEC_PROCESS_GAS) {
    input[n_input].sensor_id = BSEC_INPUT_GASRESISTOR;
    input[n_input].signal = new_data.gas_resistance;
    input[n_input].time_stamp = now;
    n_input++;
    // printf("gas:%f\n",new_data.gas_resistance);
  }
  if (bme680_settings.process_data & BSEC_PROCESS_TEMPERATURE) {
    input[n_input].sensor_id = BSEC_INPUT_TEMPERATURE;
    input[n_input].signal = new_data.temperature;
    input[n_input].time_stamp = now;
    n_input++;
    input[n_input].sensor_id = BSEC_INPUT_HEATSOURCE;
    input[n_input].signal = 0.5f;
    input[n_input].time_stamp = now;
    n_input++;
    // printf("temp:%f\n",new_data.temperature );
  }
  if (bme680_settings.process_data & BSEC_PROCESS_HUMIDITY) {
    input[n_input].sensor_id = BSEC_INPUT_HUMIDITY;
    input[n_input].signal = new_data.humidity;
    input[n_input].time_stamp = now;
    n_input++;
    // printf("humidity:%f\n",new_data.humidity );
  }
  if (bme680_settings.process_data & BSEC_PROCESS_PRESSURE) {
    input[n_input].sensor_id = BSEC_INPUT_PRESSURE;
    input[n_input].signal = new_data.pressure;
    input[n_input].time_stamp = now;
    n_input++;
  }
  bsec_library_return_t status = bsec_do_steps(input, n_input, output, &n_output);
  assert(status == BSEC_OK);
  for (int i = 0; i < n_output; i++) {
    printf("%i:%.2f,acc:%i\n", output[i].sensor_id, output[i].signal, output[i].accuracy);
    switch (output[i].sensor_id) {
      case BSEC_OUTPUT_STATIC_IAQ:
        if (output[i].accuracy >= 2) *VOC = output[i].signal;
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE: *temp = output[i].signal; break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY: *humidity = output[i].signal; break;
      case BSEC_OUTPUT_RAW_PRESSURE: *pressure = output[i].signal; break;
      default: printf("Invalid BSECBME case! %i\n", output[i].sensor_id); break;
    }
  }
  return 0;
}