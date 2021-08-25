#include <stdio.h>
#include <stdlib.h>

#include "HDC2080.h"

int main(){
    float temperature, humidity; 

	if(setup_hdc2080() != 0 ) return 1; 

	if(read_from_hdc2080(&temperature, &humidity)!=0) return 1;

	printf("{ \"temperature\" : %.1f, \"humidity\" : %.1f }\n", temperature, humidity);

    return 0;
}