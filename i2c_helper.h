#include <sys/ioctl.h>				//change slave
#include <linux/i2c-dev.h>		


//make sure recv is big enough!
int read_from_address(const int file_handle, uint8_t address, uint8_t* recv, int8_t num_of_bytes)
{
    if(recv==nullptr) abort();

    //write address
	if(write(file_handle, &address, 1) != 1)
	{
		printf("Device failed to ACK the register address %i\n",address);
		return -1;
	}
	if(read(file_handle,buff,2) != 2)
	{
		printf("Device failed to ACK the read -- maybe you are reading invalid register?\n");
		return -2;
	}
	
	*recv = (buff[1]<<8) | buff[0]; // 2 8-bit value combine to 1 16-bit 
	return 0;
}
