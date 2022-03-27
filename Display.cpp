#include "Display.h"


int display_init()
{
    if(DEV_ModuleInit() != 0) 
	{
		printf("Dev Module Info Fail!\n"); return -1;
	}
	OLED_1in5_rgb_Init();

	DEV_Delay_ms(500);	
	// 0.Create a new image cache
	UBYTE *BlackImage;
	UWORD Imagesize = (OLED_1in5_RGB_WIDTH*2) * OLED_1in5_RGB_HEIGHT;
	if((BlackImage = (UBYTE *)malloc(Imagesize + 300)) == NULL) {
			printf("Failed to apply for black memory...\r\n");
			return -1;
	}
	printf("Paint_NewImage\r\n");
	Paint_NewImage(BlackImage, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);	
	Paint_SetScale(65);
	printf("Drawing\r\n");
	//1.Select Image
	Paint_SelectImage(BlackImage);
	DEV_Delay_ms(500);
	Paint_Clear(BLACK);
	
		
		Paint_DrawString_EN(10, 0, "Unit Booting!!!", &Font16, BLACK, GREEN);
        Paint_DrawString_EN(10, 20, "Starting up...!!!", &Font16, BLACK, GREEN);
		OLED_1in5_rgb_Display(BlackImage);

        return 0;
}