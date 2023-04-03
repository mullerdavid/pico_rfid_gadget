#include <stdio.h>
#include <stdlib.h> // malloc() free()
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"

#include "lib/7941w.h"

#include "DEV_Config.h"
#include "Debug.h"
#include "GUI_Paint.h"
#include "LCD_1in3.h"

#include "ImageData.h"

static uint32_t core1_stack_static[PICO_CORE1_STACK_SIZE / sizeof(uint32_t)];

void draw_menu(size_t argc, const char** argv, size_t selected, size_t* start) 
{
    // 1*Font24 plus 9*Font20 rows fit well on the 240p height (10 lines menu)
    // 13*Font24 or 16*Font20 fit on the 240p width
    
    // Paint API has multiple bugs
    // Paint_DrawChar: FONT_BACKGROUND (white) bg is not drawn, need manual coloring before in case of white
    // Paint_DrawString_EN: bg and fg is swapped

    const uint8_t maxnum = 10;
    const uint8_t spacing = 3;
    char buff[32];
    uint8_t x = 0;
    uint8_t y = 0;
    size_t start_missing = 0;
    
    if (argc <= selected)
    {
        selected = (argc-1);
    }
    if (start == NULL)
    {
        start = &start_missing;
    }

    if (selected <= (*start))
    {
        *start = (0 == selected) ? selected : (selected - 1);
    }
    if (((*start)+maxnum-1) <= selected )
    {
        *start = ((argc-1) == selected) ? (selected - maxnum + 1) : (selected - maxnum + 2);
    }

    Paint_Clear(BLACK);
    x+=spacing;

    uint8_t i;
    for (i=*start;i<((*start)+maxnum);i++)
    {
        y+=spacing;
        const char* current = argv[i];

        if (i==selected)
        {
            snprintf(buff, 14, current);
            //need fill as Paint_DrawString_EN is not drawing white bg
            Paint_DrawRectangle(x-2, y-1, LCD_1IN3.WIDTH-spacing+2, y+24+1, WHITE, 1, DRAW_FILL_FULL ); 
            //Paint_DrawRectangle(x-2, y-1, x+(Font24.Width)*strlen(buff)+2, y+24+1, WHITE, 1, DRAW_FILL_FULL ); 
            Paint_DrawString_EN(x, y, buff, &Font24, WHITE, BLACK); 
            y+=24;
        }
        else
        {
            snprintf(buff, 17, current);
            Paint_DrawString_EN(x, y, buff, &Font20, BLACK, WHITE);
            y+=20;
        }
    }
}

void SET_Button_PIN(uint8_t PIN)
{
    DEV_GPIO_Mode(PIN, 0);
    gpio_pull_up(PIN); //Need to pull up
}

int main_core0() 
{
    /* Buttons */
    
    uint8_t keyA = 15; 
    uint8_t keyB = 17; 
    uint8_t keyX = 19; 
    uint8_t keyY = 21;
    uint8_t up = 2;
	uint8_t down = 18;
	uint8_t left = 16;
	uint8_t right = 20;
	uint8_t ctrl = 3;
    const uint8_t keys[] = {keyA, keyB, keyX, keyY, up, down, left, right, ctrl};
    {
        int i;
        for (i=0;i<count_of(keys);i++)
        {
            uint8_t key = keys[i];
            SET_Button_PIN(key);
        }
    }

    /* LCD Init */
    printf("1.3inch LCD demo...\r\n");
    LCD_1IN3_Init(HORIZONTAL);
    LCD_1IN3_Clear(WHITE);
    
    //LCD_SetBacklight(1023);
    UDOUBLE Imagesize = LCD_1IN3_HEIGHT*LCD_1IN3_WIDTH*2;
    UWORD *FrameBuffer;
    if((FrameBuffer = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }
    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage((UBYTE *)FrameBuffer,LCD_1IN3.WIDTH,LCD_1IN3.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_Clear(WHITE);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(WHITE);
    
    // /* GUI */
    printf("drawing...\r\n");
    
#if 0
     Paint_DrawImage(gImage_1inch3_1,0,0,240,240);
     LCD_1IN3_Display(FrameBuffer);
     DEV_Delay_ms(2000);
#endif
#if 0
    Paint_DrawPoint(2,1, BLACK, DOT_PIXEL_1X1,  DOT_FILL_RIGHTUP);//240 240
    Paint_DrawPoint(2,6, BLACK, DOT_PIXEL_2X2,  DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2,11, BLACK, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2,16, BLACK, DOT_PIXEL_4X4, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2,21, BLACK, DOT_PIXEL_5X5, DOT_FILL_RIGHTUP);
    Paint_DrawLine( 10,  5, 40, 35, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine( 10, 35, 40,  5, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    Paint_DrawLine( 80,  20, 110, 20, CYAN, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine( 95,   5,  95, 35, CYAN, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    Paint_DrawRectangle(10, 5, 40, 35, RED, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(45, 5, 75, 35, BLUE, DOT_PIXEL_2X2,DRAW_FILL_FULL);

    Paint_DrawCircle(95, 20, 15, GREEN, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(130, 20, 15, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);


    Paint_DrawNum (50, 40 ,9.87654321, &Font20,3,  WHITE,  BLACK);
    Paint_DrawString_EN(1, 40, "ABC", &Font20, 0x000f, 0xfff0);
    Paint_DrawString_EN(1, 100, "WaveShare", &Font16, RED, WHITE); 
    Paint_DrawString_EN(1, 120, "0123456789", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(1, 150, "1111111111", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(1, 180, "0000000000", &Font24, WHITE, BLACK);

    // /*3.Refresh the picture in RAM to LCD*/
    LCD_1IN3_Display(FrameBuffer);
    DEV_Delay_ms(2000);

#endif
#if 1


    const char* menu[] = {
        "Menuitem 1",
        "Menuitem 2",
        "Menuitem 3",
        "Menuitem 4",
        "Menuitem 5",
        "Menuitem 6 this is very looong",
        "Menuitem 7",
        "Menuitem 8",
        "Menuitem 9",
        "Menuitem 10",
        "11 Lorem ipsum",
        "12 dolor sit amet",
        "13 consectetur",
        "14 adipiscing elit",
        "15 sed do eiusmod",
        "16  tempor incididunt"
    };

    const uint32_t repeat_ms = 250;

    bool changed = true;
    size_t start = 0;
    size_t selected = 0;
    uint32_t states[24] = {0};
    uint32_t states_prev[24] = {0};

    while(1){
        int i;
        uint32_t now = (time_us_64() / 1000);
        for (i=0;i<count_of(keys);i++)
        {
            uint8_t key = keys[i];
            bool down = ! DEV_Digital_Read(key); //pullups
            if (!down)
            {
                states[key] = 0;
            }
            if (down && !states[key])
            {
                states[key] = now;
            }
            if (down && repeat_ms<(now-states_prev[key]))
            {
                states_prev[key] = 0;
            }
        }

        if( states[up] && !states_prev[up] && 0<selected){
            selected--;
            changed = true;
        }
        if( states[down] && !states_prev[down] && selected<(count_of(menu)-1)){
            selected++;
            changed = true;
        }
        if( states[ctrl] && !states_prev[ctrl]){
            break;
        }

        if (changed)
        {
            draw_menu(count_of(menu), menu, selected, &start);
            LCD_1IN3_Display(FrameBuffer);
            changed = false;
        }
        
        for (i=0;i<count_of(keys);i++)
        {
            uint8_t key = keys[i];
            states_prev[key] = states[key];
        }
        sleep_ms(15);
    }
    sleep_ms(1000);

#endif
#if 1

    Paint_Clear(WHITE);
    Paint_DrawRectangle(208, 15, 237, 45, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(208, 75, 237, 105, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(208, 135, 237, 165, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(208, 195, 237, 225, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(60, 60, 91, 90, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(60, 150, 91, 180, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(15, 105, 46, 135, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(105, 105, 136, 135, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    Paint_DrawRectangle(60, 105, 91, 135, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
    LCD_1IN3_Display(FrameBuffer);

    
    while(1){
        if(DEV_Digital_Read(keyA ) == 0){
            Paint_DrawRectangle(208, 15, 236, 45, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(208, 15, 236, 45,FrameBuffer);
            //printf("gpio =%d\r\n",keyA);
        }
        else{
            Paint_DrawRectangle(208, 15, 236, 45, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(208, 15, 236, 45,FrameBuffer);
        }
            
        if(DEV_Digital_Read(keyB ) == 0){
            Paint_DrawRectangle(208, 75, 236, 105, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(208, 75, 236, 105,FrameBuffer);
            //printf("gpio =%d\r\n",keyB);
        }
        else{
            Paint_DrawRectangle(208, 75, 236, 105, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(208, 75, 236, 105,FrameBuffer);
        }
        
        if(DEV_Digital_Read(keyX ) == 0){
            Paint_DrawRectangle(208, 135, 236, 165, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(208, 135, 236, 165,FrameBuffer);
            //printf("gpio =%d\r\n",keyX);
        }
        else{
            Paint_DrawRectangle(208, 135, 236, 165, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(208, 135, 236, 165,FrameBuffer);
        }
            
        if(DEV_Digital_Read(keyY ) == 0){
            Paint_DrawRectangle(208, 195, 236, 225, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(208, 195, 236, 225,FrameBuffer);
            //printf("gpio =%d\r\n",keyY);
        }
        else{
            Paint_DrawRectangle(208, 195, 236, 225, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(208, 195, 236, 225,FrameBuffer);
        }


        if(DEV_Digital_Read(up ) == 0){
            Paint_DrawRectangle(60, 60, 90, 90, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(60, 60, 90, 90,FrameBuffer);
            //printf("gpio =%d\r\n",up);
        }
        else{
            Paint_DrawRectangle(60, 60, 90, 90, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(60, 60, 90, 90,FrameBuffer);
        }

        if(DEV_Digital_Read(down ) == 0){
            Paint_DrawRectangle(60, 150, 90, 180, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(60, 150, 90, 180,FrameBuffer);
            //printf("gpio =%d\r\n",dowm);
        }
        else{
            Paint_DrawRectangle(60, 150, 90, 180, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(60, 150, 90, 180,FrameBuffer);
        }
        
        if(DEV_Digital_Read(left ) == 0){
            Paint_DrawRectangle(15, 105, 45, 135, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(15, 105, 45, 135,FrameBuffer);
            //printf("gpio =%d\r\n",left);
        }
        else{
            Paint_DrawRectangle(15, 105, 45, 135, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(15, 105, 45, 135,FrameBuffer);
        }
            
        if(DEV_Digital_Read(right ) == 0){
            Paint_DrawRectangle(105, 105, 135, 135, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(105, 105, 135, 135,FrameBuffer);
            //printf("gpio =%d\r\n",right);
        }
        else{
            Paint_DrawRectangle(105, 105, 135, 135, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(105, 105, 135, 135,FrameBuffer);
        }
        
        if(DEV_Digital_Read(ctrl ) == 0){
            Paint_DrawRectangle(60, 105, 90, 135, 0xF800, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(60, 105, 90, 135,FrameBuffer);
            //printf("gpio =%d\r\n",ctrl);
        }
        else{
            Paint_DrawRectangle(60, 105, 90, 135, WHITE, DOT_PIXEL_2X2,DRAW_FILL_FULL);
            LCD_1IN3_DisplayWindows(60, 105, 90, 135,FrameBuffer);
        }

    }

#endif

    /* Module Exit */
    free(FrameBuffer);
    FrameBuffer = NULL;
    

    while(1){
	    //static int counter = 0;
        //printf("[core0] At cycle %d\n", counter++);
        sleep_ms(1000);
    }
    
    DEV_Module_Exit();
    return 0;
}

void main_core1() 
{
    while (true) {
	    //static int counter = 0;
        //printf("[core1] At cycle %d\n", counter++);
        
        uint64_t id = rfid_7941w_alt_read_id(uart1);

        if (id != 0)
        {
            printf("Received id: %010lu\n", (uint32_t)id);
            sleep_ms(500);
        }

        sleep_ms(100);
        sleep_ms(800);
    }
}

int main() 
{
    int ret;

    stdio_init_all();
    
    sleep_ms(100);

    // Also sets up I2C on Pin 6/7 which is unused for the lcd board
    bool lcd_board_error = DEV_Module_Init();

    // Need to come after DEV_Module_Init to reinit Pin 6/7 for uart1
    rfid_7941w_init(uart1);
    
    multicore_launch_core1_with_stack(main_core1, core1_stack_static, sizeof(core1_stack_static));

    if(lcd_board_error)
    { 
        sleep_ms(60000);
        ret = -1;
    }
    else
    {
        DEV_SET_PWM(50);
        ret = main_core0();
    }
    return ret;
    
}