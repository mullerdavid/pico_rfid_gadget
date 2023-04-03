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

//TODO: CPP implementation

static uint32_t core1_stack_static[PICO_CORE1_STACK_SIZE / sizeof(uint32_t)];

typedef struct read_struct {
    rfid_7941w_type_t type;
    uint8_t vendor;
    uint32_t id;
} read_struct_t;

static volatile read_struct_t read_buffer[64];
static volatile size_t read_buffer_counter;

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
    for (i=*start;i<((*start)+maxnum) && i<argc;i++)
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

typedef enum app_state {
  MAIN,
  READ,
  WRITE_LF,
  WRITE_HF
} app_state_t;



void main_core1() 
{
    while (true) {
	    //static int counter = 0;
        //printf("[core1] At cycle %d\n", counter++);
        
        rfid_7941w_type_t info;
        uint64_t id = rfid_7941w_alt_read_id_with_info(uart1, &info);

        if (info != ERROR)
        {
            read_struct_t current;
            current.type = info;
            current.vendor = (uint8_t)(id >> 32);
            current.id = (uint32_t)id;
            printf("Received id: %010lu\n", current.id);

            const size_t rb_size = count_of(read_buffer);
            size_t last_index = (0<read_buffer_counter ? (read_buffer_counter-1) % rb_size : 0);
            size_t new_index = read_buffer_counter % rb_size;
            volatile read_struct_t *last = &read_buffer[last_index];
            volatile read_struct_t *new = &read_buffer[new_index];

            if (last->type != current.type || last->vendor != current.vendor || last->id != current.id)
            {
                printf(" Adding to buffer. %u\n", read_buffer_counter);
                new->type = current.type;
                new->vendor = current.vendor;
                new->id = current.id;
                read_buffer_counter++;
            }

            sleep_ms(500);
        }

        sleep_ms(200);
    }
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

    const char* main_menu[] = {
        "Read",
        "Write 125 khz",
        "Write 13.36 mhz"
    };
    const char* read_menu[] = {
        "12314514123414",
        "adsasdasasd",
        "adsasdasasd",
        "adsasdasasd",
        "14fsdsdfsdga"
    };

    const uint32_t repeat_ms = 250;

    bool changed = true;
    size_t start = 0;
    size_t selected = 0;
    uint32_t states[24] = {0};
    uint32_t states_prev[24] = {0};
    app_state_t app = MAIN;

    size_t read_buffer_counter_prev;


    while(1){
        const char** menu;
        size_t menu_count;
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

        if (app == MAIN)
        {
            menu = main_menu;
            menu_count = count_of(main_menu);

            if((states[ctrl] && !states_prev[ctrl]) || (states[keyA] && !states_prev[keyA]))
            {
                switch (selected)
                {
                case 0:
                    {
                        changed = true;
                        app = READ;
                        selected = 0;
                        read_buffer_counter_prev = 0;
                        multicore_launch_core1_with_stack(main_core1, core1_stack_static, sizeof(core1_stack_static));
                        continue;
                    }
                    break;
                
                default:
                    break;
                }
                printf("CTRL\n");
            }
        }
        else if (app == READ)
        {
            if (read_buffer_counter_prev != read_buffer_counter)
            {
            }

            menu_count = count_of(read_menu);
            menu = read_menu;

            if( states[keyB] && !states_prev[keyB])
            {
                multicore_reset_core1();
                changed = true;
                app = MAIN;
                selected = 0;
                continue;
            }
        }

        if( states[up] && !states_prev[up] && 0<selected){
            selected--;
            changed = true;
        }
        if( states[down] && !states_prev[down] && selected<(menu_count-1)){
            selected++;
            changed = true;
        }

        if (changed)
        {
            draw_menu(menu_count, menu, selected, &start);
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


    /* Module Exit */
    free(FrameBuffer);
    FrameBuffer = NULL;
    
    DEV_Module_Exit();
    return 0;
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