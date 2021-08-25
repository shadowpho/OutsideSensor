#include <stdio.h>
#include <stdlib.h>

#include "HDC2080.h"
#include "i2c_helper.h"

int main(){
	if(setup_i2C()!=0)
	{
		printf("Failed to open I2c file\n");
		return -1;
	}

    float temperature, humidity; 

	if(setup_hdc2080() != 0 ) return 1; 

	if(read_from_hdc2080(&temperature, &humidity)!=0) return 1;

	printf("{ \"temperature\" : %.1f, \"humidity\" : %.1f }\n", temperature, humidity);

    return 0;
}