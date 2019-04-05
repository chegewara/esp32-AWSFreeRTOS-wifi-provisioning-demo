#ifndef __M5_STACK_MENU_H__
#define __M5_STACK_MENU_H__

#include "esp_err.h"

#define BUTTON_1 39 // LEFT BUTTON
#define BUTTON_2 38 // MID BUTTON 
#define BUTTON_3 37 // RIGHT BUTTON
#define MAX_MENUS 10


// menu item is structure for single menu with 3 button actions and optional argument passed to callback
typedef void (* menu_callback)(void* arg, uint8_t btn);
typedef struct {
	char* buttons[3];
	menu_callback callbacks[3];
	char* title;
    void* arg;
}menu_item_t;

typedef struct {
	uint8_t type; // image, char
	union{
		uint8_t* image;
		char text;
	}data;
	size_t len;
}overlay_item_t;

esp_err_t init_menu();
void next_menu(void* arg, uint8_t btn);
void prev_menu(void* arg, uint8_t btn);
void menu_add_item(menu_item_t* item);
// menu_item_t* get_active_menu();
void disp_header(char *info);
void clear_menu();
void draw_menu();
void display_splash_screen(color_t);
int get_active_menu();
void set_active_menu();
bool tft_callback_task();

void add_overlay_item(overlay_item_t* item, uint8_t pos);
void delete_overlay_item();

#endif
