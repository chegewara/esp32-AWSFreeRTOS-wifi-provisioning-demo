#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "tftspi.h"
#include "tft.h"

#include "m5stack_menu.h"
#include "m5stack_logo_jpg.h"

// static void disp_button();
static int items_count = 0;
static int selected_menu = 0;

static xQueueHandle xRxQueue = NULL;
static menu_item_t* menus[MAX_MENUS];
static menu_item_t* current_menu;
static overlay_item_t* overlays[MAX_MENUS];
static void disp_overlays();

static void IRAM_ATTR isr_button0_pressed(void *args)
{
	BaseType_t xHigherPriorityTaskWoken;
	uint8_t btn = 0;
    xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR( xRxQueue, &btn, &xHigherPriorityTaskWoken );
    if( xHigherPriorityTaskWoken )
    {
        portYIELD_FROM_ISR();
    }
}

static void IRAM_ATTR isr_button1_pressed(void *args)
{
	BaseType_t xHigherPriorityTaskWoken;
	uint8_t btn = 1;
    xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR( xRxQueue, &btn, &xHigherPriorityTaskWoken );
    if( xHigherPriorityTaskWoken )
    {
        portYIELD_FROM_ISR();
    }
}

static void IRAM_ATTR isr_button2_pressed(void *args)
{
	BaseType_t xHigherPriorityTaskWoken;
	uint8_t btn = 2;
    xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR( xRxQueue, &btn, &xHigherPriorityTaskWoken );
    if( xHigherPriorityTaskWoken )
    {
        portYIELD_FROM_ISR();
    }
}

bool tft_callback_task()
{
	bool ret = false;
	uint8_t btn;
	if( xQueueReceive( xRxQueue, ( void * ) &btn, 0) == pdTRUE)
	{
		// todo add debouncer
		taskENTER_CRITICAL();
		vTaskDelay(50/portTICK_PERIOD_MS);
		if(current_menu->callbacks[btn] != NULL)
			current_menu->callbacks[btn](current_menu->arg, btn);

		taskEXIT_CRITICAL();
		ret = true;
	}
	return ret;
}

/**
 * @brief initialize buttons
 * 
 */
esp_err_t init_menu()
{
	esp_err_t err;
	xRxQueue = xQueueCreate(1, 1);
	//Configure buttons
    gpio_config_t btn_config;
    btn_config.intr_type = GPIO_INTR_NEGEDGE;    
    btn_config.mode = GPIO_MODE_INPUT;           
    btn_config.pin_bit_mask = ((((uint64_t) 1) << BUTTON_1) | (((uint64_t) 1) << BUTTON_2) | (((uint64_t) 1) << BUTTON_3));
    btn_config.pull_up_en = GPIO_PULLUP_DISABLE;    
    btn_config.pull_down_en = GPIO_PULLDOWN_DISABLE; 
    err = gpio_config(&btn_config);

    //Configure interrupt and add handler
	if(err == ESP_OK)
	    err = gpio_install_isr_service(0);                  //Start Interrupt Service Routine service

	if(err != ESP_OK) ESP_LOGE(__func__, "error %d", err);
	
	if(err == ESP_OK){
		err |= gpio_isr_handler_add(BUTTON_1, isr_button0_pressed, NULL); //Add handler of interrupt
		err |= gpio_isr_handler_add(BUTTON_2, isr_button1_pressed, NULL); //Add handler of interrupt
		err |= gpio_isr_handler_add(BUTTON_3, isr_button2_pressed, NULL); //Add handler of interrupt
	}

	return err;
}

//-----------------------------------------------------------------------//
static void disp_buttons();

static void draw_selector()
{
	TFT_saveClipWin();
	TFT_setclipwin(0, 39, 15, _height);
	TFT_fillWindow(TFT_BLACK);
	TFT_setFont(UBUNTU16_FONT, NULL);
	for(int i=0;i<MAX_MENUS;i++)
	{
		if(menus[i] != NULL && menus[i]->title != NULL)
		{
			TFT_print("", 0, i*TFT_getfontheight()+2);
			if(menus[i] == current_menu)
				TFT_print(">", 1, LASTY);
		}
	}

	TFT_restoreClipWin();
	disp_buttons();
}

void next_menu(void* arg, uint8_t btn)
{
	portENTER_CRITICAL();
	if(selected_menu < items_count-1)
	{
		selected_menu++;
	}
	else
	{
		selected_menu = 0;
	}
	
	current_menu = menus[selected_menu];
	portEXIT_CRITICAL();
	draw_selector();
}

void prev_menu(void* arg, uint8_t btn)
{
	portENTER_CRITICAL();
	if(selected_menu > 0)
	{
		selected_menu--;
	}
	else
	{
		selected_menu = items_count - 1;
	}
	
	current_menu = menus[selected_menu];
	portEXIT_CRITICAL();
	draw_selector();
}

void menu_add_item(menu_item_t* item)
{
	if(items_count < MAX_MENUS){
		menus[items_count++] = item;
		if(current_menu == NULL)
			current_menu = item;
	}
}

void clear_menu()
{
	memset(&menus, (menu_item_t*)NULL, MAX_MENUS*4);
	items_count = 0;
	current_menu = NULL;
	selected_menu = 0;
	// TFT_fillScreen(TFT_BLUE);
}

void add_overlay_item(overlay_item_t* item, uint8_t pos)
{
	overlays[pos] = item;
	// disp_overlays();
}

void delete_overlay_item(overlay_item_t* item)
{
	for(int i=0;i<MAX_MENUS;i++)
	{
		if(overlays[i] != NULL && overlays[i] == item)
		{
			overlays[i] = NULL;
			break;
		}
	}
}

//-----------------------------------------------------------------------//

//------------------------------------------------------------------------------

/**
 * @brief display header text
 * 
 */
void disp_header(char* info)
{
	TFT_resetclipwin();

	_fg = TFT_YELLOW;
	_bg = (color_t){ 64, 64, 64 };

    if (_width < 240) TFT_setFont(DEF_SMALL_FONT, NULL);
	else TFT_setFont(DEFAULT_FONT, NULL);
	TFT_fillRect(0, 0, _width, TFT_getfontheight()+8, _bg);
	TFT_drawRect(0, 0, _width, TFT_getfontheight()+8, TFT_CYAN);

    font_rotate = 0;
	if(info != NULL)
		TFT_print(info, CENTER, 4);

	// disp_overlays();
	_bg = TFT_BLACK;
	TFT_setclipwin(0,TFT_getfontheight()+2, _width-1, _height-TFT_getfontheight()-12);
}

static void update_header(char *hdr, char *ftr)
{
	// color_t last_fg, last_bg;

	// TFT_saveClipWin();
	// TFT_resetclipwin();

	// Font curr_font = cfont;
	// last_bg = _bg;
	// last_fg = _fg;
	// _fg = TFT_YELLOW;
	// _bg = (color_t){ 64, 64, 64 };
    // if (_width < 240) TFT_setFont(DEF_SMALL_FONT, NULL);
	// else TFT_setFont(DEFAULT_FONT, NULL);

	// if (hdr) {
    //     font_rotate = 0;
	// 	TFT_fillRect(1, 1, _width-3, TFT_getfontheight()+6, _bg);
	// 	TFT_print(hdr, CENTER, 4);
	// }

	// if (ftr) {
	// 	TFT_fillRect(1, _height-TFT_getfontheight()-8, _width-3, TFT_getfontheight()+6, _bg);
    //     font_rotate = 0;
	// 	if (strlen(ftr) == 0){}
	// 	else TFT_print(ftr, CENTER, _height-TFT_getfontheight()-5);
	// }

	// cfont = curr_font;
	// _fg = last_fg;
	// _bg = last_bg;

	// TFT_restoreClipWin();
}

static void disp_buttons()
{
	color_t last_fg, last_bg;

	TFT_saveClipWin();
	TFT_setclipwin(0, _height-TFT_getfontheight()-11, _width, _height);

	text_wrap = 1;
	Font curr_font = cfont;
	last_bg = _bg;
	last_fg = _fg;
	_fg = TFT_YELLOW;
	_bg = (color_t){ 64, 64, 64 };
    if (_width < 240) TFT_setFont(DEF_SMALL_FONT, NULL);
	else TFT_setFont(DEFAULT_FONT, NULL);

	TFT_fillRect(0, 0, _width, _height, TFT_BLACK);

	for(int pos=0;pos<3;pos++){
		char* title = NULL;
		if(current_menu != NULL)
		title = current_menu->buttons[pos];
		else title = "err";
		if(title == NULL)
			continue;

		TFT_fillRect(0, _height-TFT_getfontheight()-11, _width, _height, TFT_BLACK);
		TFT_setclipwin(pos*_width/3+3, _height-TFT_getfontheight()-10, (pos+1)*_width/3-3, _height-2);
		TFT_fillWindow(_bg);
		TFT_drawRect(0, 0, 100, 100, TFT_CYAN);
		TFT_print(title, CENTER, CENTER);
	}

	text_wrap = 0;
	cfont = curr_font;
	_fg = last_fg;
	_bg = last_bg;

	TFT_restoreClipWin();
}

static void disp_overlays()
{
	TFT_saveClipWin();
	TFT_resetclipwin();
	for(int i=0;i<6;i++)
	{
		overlay_item_t* o = overlays[i];
		if(o != NULL){
			// ESP_LOGI(__func__, "overlay");
			if(o->type == 0){
				TFT_jpg_image(5 + i*30, 3, 0, NULL, o->data.image, o->len);
			}
			else
			{
				TFT_print(&o->data.text, 10, 3);
			}
			
		}
		else{
			color_t c = (color_t){ 64, 64, 64 };
			TFT_fillRect(5 + i*30, 3, 30, 30, _bg);
		}
	}
	TFT_restoreClipWin();
}

void draw_menu()
{
	TFT_saveClipWin();
	TFT_setclipwin(0, 39, _width, _height);
	TFT_fillWindow(TFT_BLACK);
	TFT_setFont(UBUNTU16_FONT, NULL);
	for(int i=0;i<MAX_MENUS;i++)
	{
		if(menus[i] != NULL && menus[i]->title != NULL)
		{
			TFT_print(menus[i]->title, 15, i*TFT_getfontheight()+2);
			if(menus[i] == current_menu)
				TFT_print(">", 1, LASTY);
		}
	}

	TFT_restoreClipWin();
	disp_buttons();
}

void display_splash_screen(color_t bg)
{
	TFT_fillScreen(bg);
	TFT_jpg_image(CENTER, CENTER, 0, NULL, m5stack_logo_jpg, m5stack_logo_jpg_len);
}

int get_active_menu()
{
	return selected_menu;
}

void set_active_menu(int menu)
{
	selected_menu = menu;
	current_menu = menus[selected_menu];
}
