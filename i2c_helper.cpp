#include <sys/ioctl.h>				//change slave
#include <linux/i2c-dev.h>		
#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>	
#include <mutex>

#include <errno.h>

static int file_i2c_handle =0;
static std::mutex i2c_mutex;

int setup_i2C()
{
	//OPEN I2C communication 
	char *filename = (char*)"/dev/i2c-1";
	if ((file_i2c_handle = open(filename, O_RDWR)) < 0)
	{
		printf("Failed to open the i2c bus\n");
		return -1;
	}
	return 0;
}

//make sure recv is big enough!
int communicate_I2C(uint8_t device_address,bool write_comm, uint8_t register_address, uint8_t* recv_buff, int8_t num_of_bytes)
{
	const std::lock_guard<std::mutex> lock(i2c_mutex);
    int ret=0; //debug
    uint8_t buff[4]; //xxx sigh
    if(write_comm == true && num_of_bytes>3) 
    {  
        printf("Writeing >3 bytes not supported yet");
        return -9;
    }

    if(recv_buff==nullptr) abort();

	if (ioctl(file_i2c_handle, I2C_SLAVE,device_address ) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave.\n");
		return -3;
	}
    if(write_comm == false)
    {
        if(write(file_i2c_handle, &register_address, 1) != 1)
        {
            printf("Device failed to ACK the register address %i\n",register_address);
            return -1;
        }

		if(read(file_i2c_handle,recv_buff,num_of_bytes) != num_of_bytes)
		{
			printf("Device failed to ACK the read -- maybe you are reading invalid register?\n");
			return -2;
		}
    }
	if(write_comm==true)
    {
        buff[0] = register_address;
        for(int i=0;i<num_of_bytes;i++)
            buff[i+1] = recv_buff[i];

		if((ret=write(file_i2c_handle,buff,num_of_bytes+1)) != (num_of_bytes+1))
		{
            perror("error:");
			printf("Device failed to ACK the write -- maybe you are reading invalid register, output:%i ?\n",ret);
			return -4;
		}
    } 
	return 0;
}
