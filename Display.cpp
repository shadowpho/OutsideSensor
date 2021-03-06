#include "Display.h"
#include <assert.h>
#include <ctime>

UBYTE *BlackImage;

int display_init()
{
    if(DEV_ModuleInit() != 0) 
	{
		printf("Dev Module Info Fail!\n"); return -1;
	}
	OLED_1in5_rgb_Init();

	DEV_Delay_ms(500);	
	// 0.Create a new image cache
	
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
	
		
		Paint_DrawString_EN(0, 0, "Unit", &Font16, BLACK, GREEN);
        Paint_DrawString_EN(0, 16, "booting!", &Font16, BLACK, GREEN);
        
        Paint_DrawString_EN(0, 48, "Starting", &Font16, BLACK, GREEN);
        Paint_DrawString_EN(0, 64, "up!", &Font16, BLACK, GREEN);
		OLED_1in5_rgb_Display(BlackImage);

        return 0;
}
char buff[128];

void draw_row(const char* txt, float value, int row, float yellow_limit, float red_limit)
{
    assert(row>=0);
    assert(row<8);
    uint16_t color = GREEN;
    if(value > red_limit)
        color = RED;
    else if(value > yellow_limit)
        color = YELLOW;
    snprintf(buff,128, txt,value);
    Paint_DrawString_EN(0, row*16, buff, &Font16, BLACK, color);

}

int display_data(float temp, float humidity, float voltage, float VOC, float NOX, float hcho, float pm1)
{
    Paint_SelectImage(BlackImage);
    Paint_Clear(BLACK);
    draw_row("Temp:  %2.1f",temp,0,25,30);
    draw_row("Humid: %2.1f",humidity,1,120,120);
    draw_row("VOC:    %3.0f",VOC,2,200,300);
    draw_row("NOX:    %3.0f",NOX,3,100,150);
    draw_row("HCHO:   %3.0f",hcho,4,50,100);
    draw_row("pm1:    %3.0f",pm1,5,100,200);
    draw_row("Methane:%1.1f",voltage,6,1.0,1.5);


    std::time_t t = std::time(nullptr);
    std::strftime(buff, 80, "%R %m/%d", std::localtime(&t));

    
    Paint_DrawString_EN(0, 7*16, buff, &Font16, BLACK, BLUE);
    OLED_1in5_rgb_Display(BlackImage);
    return 0;
}