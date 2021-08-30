#include <sys/ioctl.h> //change slave
#include <linux/i2c-dev.h>
#include <unistd.h> //Needed for I2C port
#include <fcntl.h>
#include <mutex>
#include <cassert>
#include <chrono>
#include <thread>

#include <errno.h>
#include <linux/i2c.h>

static int file_i2c_handle = 0;
static std::mutex i2c_mutex;

void sleep_ms(uint32_t sleep_ms)
{

	std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
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

//make sure recv is big enough!
//NOTE: RASP PI DOES NOT SUPPORT I2C_M_NOSTART!!!!!
int communicate_I2C(uint8_t device_address, bool write_comm, uint8_t register_address, uint8_t *recv_buff, int8_t num_of_bytes)
{
	const std::lock_guard<std::mutex> lock(i2c_mutex);

	if (write_comm == true && num_of_bytes > 6)
	{
		printf("Writing >6 bytes not supported yet");
		return -9;
	}
	assert(recv_buff != nullptr);

	uint8_t buff[8];

	buff[0] = register_address;

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
