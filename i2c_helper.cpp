#include <sys/ioctl.h> //change slave
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h> //Needed for I2C port
#include <fcntl.h>
#include <cassert>
#include <chrono>
#include <thread>
#include <errno.h>
#include "i2c_helper.h"

static int file_i2c_handle = 0;
static std::mutex i2c_mutex;

void add_to_CMA(CMA_Data *struct_data, float val1, float val2, float val3)
{
	const std::lock_guard<std::mutex> lock(struct_data->data_mutex);
	struct_data->CMA_value1 += (double)val1;
	struct_data->CMA_value2 += (double)val2;
	struct_data->CMA_value3 += (double)val3;
	struct_data->num_of_samples++;
}
void remove_CMA(CMA_Data *struct_data, float* val1, float* val2, float* val3)
{
	const std::lock_guard<std::mutex> lock(struct_data->data_mutex);
	float ret_value1 = 0;
	float ret_value2 = 0;
	float ret_value3 = 0;
	if (struct_data->num_of_samples != 0)
	{
		ret_value1 = struct_data->CMA_value1 / struct_data->num_of_samples;
		ret_value2 = struct_data->CMA_value2 / struct_data->num_of_samples;
		ret_value3 = struct_data->CMA_value3 / struct_data->num_of_samples;
	}
	if(val1!=nullptr)
		*val1 = ret_value1;
	if(val2!=nullptr)
		*val2 = ret_value2;
	if(val3!=nullptr)
		*val3 = ret_value3;	
	struct_data->num_of_samples = 0;
	struct_data->CMA_value1 = 0;
	struct_data->CMA_value2 = 0;
	struct_data->CMA_value3 = 0;
}

void sleep_ms(uint32_t sleep_ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
}
void sleep_us(uint32_t sleep_us)
{
	std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
}
int setup_i2C()
{
	//OPEN I2C communication
	char *filename = (char *)"/dev/i2c-1";
	if ((file_i2c_handle = open(filename, O_RDWR)) < 0)
	{
		printf("Failed to open the i2c bus\n");
		return -1;
	}
	return 0;
}

int read_I2C(uint8_t device_address, uint8_t* recv_buff, int8_t num_of_bytes)
{
	const std::lock_guard<std::mutex> lock(i2c_mutex);

	struct i2c_msg msgs[1];
	struct i2c_rdwr_ioctl_data msgset[1];

	msgs[0].addr = device_address;
	msgs[0].flags = I2C_M_RD;
	msgs[0].len = num_of_bytes;
	msgs[0].buf = recv_buff;

	msgset[0].msgs = msgs;
    msgset[0].nmsgs = 1;

	if (ioctl(file_i2c_handle, I2C_RDWR, &msgset) < 0)
	{
		perror("ioctl(I2C_RDWR) in i2c_comm");
		return -1;
	}

	return 0;
}


//make sure recv is big enough!
//NOTE: RASP PI DOES NOT SUPPORT I2C_M_NOSTART!!!!!
int communicate_I2C(uint8_t device_address, bool write_comm, uint8_t register_address, uint8_t *recv_buff, int8_t num_of_bytes)
{
	const std::lock_guard<std::mutex> lock(i2c_mutex);

	if (write_comm == true && num_of_bytes > 16)
	{
		printf("Writing >6 bytes not supported yet");
		return -9;
	}
	assert(recv_buff != nullptr);

	uint8_t buff[18];

	buff[0] = register_address;

	if (write_comm == true)
		for (int i = 0; i < num_of_bytes; i++)
			buff[i + 1] = recv_buff[i];

	struct i2c_msg msgs[2];
	struct i2c_rdwr_ioctl_data msgset[1];

	msgs[0].addr = device_address;
	msgs[0].flags = 0; //first byte ALWAYS write
	if (write_comm == true)
		msgs[0].len = 1 + num_of_bytes;
	else
		msgs[0].len = 1;
	msgs[0].buf = buff;

	msgs[1].addr = device_address;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = num_of_bytes;
	msgs[1].buf = recv_buff;

	msgset[0].msgs = msgs;
	if (write_comm == true)
		msgset[0].nmsgs = 1;
	else
		msgset[0].nmsgs = 2;

	if (ioctl(file_i2c_handle, I2C_RDWR, &msgset) < 0)
	{
		perror("ioctl(I2C_RDWR) in i2c_comm");
		return -1;
	}

	return 0;
}

int write_read_I2C(uint8_t device_address, uint8_t* write_buff, uint8_t num_write, uint8_t* read_buff, uint8_t num_read)
{
	const std::lock_guard<std::mutex> lock(i2c_mutex);
	assert(read_buff != nullptr);
	assert(write_buff != nullptr);

	struct i2c_msg msgs[2];
	struct i2c_rdwr_ioctl_data msgset[1];

	msgs[0].addr = device_address;
	msgs[0].flags = 0; //first byte  write
	msgs[0].len = num_write;
	msgs[0].buf = write_buff;

	msgs[1].addr = device_address;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = num_read;
	msgs[1].buf = read_buff;

	msgset[0].msgs = msgs;
	msgset[0].nmsgs = 2;

	if (ioctl(file_i2c_handle, I2C_RDWR, &msgset) < 0)
	{
		perror("ioctl(I2C_RDWR) in i2c_comm");
		return -1;
	}

	return 0;
}