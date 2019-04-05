/* TFT demo

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
// #include "./portable/FreeRTOS_POSIX_portable_default.h"
#include <time.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "tftspi.h"
#include "tft.h"
#include "aws_iot_network.h"
#include "aws_iot_network_manager.h"
#include "driver/sdspi_host.h"
#include "aws_wifi.h"
#include "iot_ble.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "esp_attr.h"
#include <sys/time.h>
#include <unistd.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "m5stack_menu.h"
#include "tft_main.h"
#include "aws_iot_network.h"
#define TAG __func__


static void start_spi_display();
WIFIReturnCode_t WIFI_Provision();

// ==========================================================
// Define which spi bus to use TFT_VSPI_HOST or TFT_HSPI_HOST
#define SPI_BUS TFT_VSPI_HOST
// ==========================================================
// extern void start_connect(device_info_t* device);
// extern void start_disconnect(device_info_t* device);
extern void publishMsg(char* msg);
extern void draw_display(bool);
#include <inttypes.h>

extern EventGroupHandle_t xEventGroup;
static xQueueHandle xMenuQueue;
static xQueueHandle xBlePinConfirm;
static  AwsIotNetworkState_t xNetworkState;
// xQueueHandle xWiFiStationsSearch;
static struct tm* tm_info;
static struct timeval tv;
struct timezone tz;
static char tmp_buff[64];
static time_t time_now = 0;

static void wifiStateCallback(uint32_t ulNetworkType, AwsIotNetworkState_t xNetworkState, void* pvContext);
static void init_sd_card(void);

#define PROJ_NAME "Provioning demo"
#define PROJ_VERSION "1.0.0"
static const char tag[] = "[TFT Demo]";

#ifdef SD_CARD_DEMO
static sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
static sdmmc_host_t host = SDSPI_HOST_DEFAULT();

static esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5
};
#endif
#include "bt_hal_manager_types.h"

//==================================================================================
static int obtain_time(void)
{
	int res = 1;
    // wait for time to be set
    int retry = 0;
    const int retry_count = 5;

    time(&time_now);
    gettimeofday(&tv, &tz);
	tm_info = localtime((time_t*)&tv.tv_sec);

    while(tm_info->tm_year < (2016 - 1900) && ++retry < retry_count) {
        //ESP_LOGI(tag, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		sprintf(tmp_buff, "Wait %0d/%d", retry, retry_count);
    	TFT_print(tmp_buff, CENTER, LASTY);
		vTaskDelay(500 / portTICK_RATE_MS);
        time(&time_now);
    	tm_info = localtime(&time_now);
    }
    if (tm_info->tm_year < (2016 - 1900)) {
    	ESP_LOGI(tag, "System time NOT set.");
    	res = 0;
    }
    else {
    	ESP_LOGI(tag, "System time is set. %02d.%02d.%04d", tm_info->tm_mday, tm_info->tm_mon+1, tm_info->tm_year+1900);
    	ESP_LOGI(tag, "System time is set. %02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    }

    return res;
}

static void _dispTime()
{
	TFT_resetclipwin();

	_bg = (color_t){ 64, 64, 64 };

	Font curr_font = cfont;
    if (_width < 240) TFT_setFont(DEF_SMALL_FONT, NULL);
	else TFT_setFont(DEFAULT_FONT, NULL);
    TFT_fillRect(_width-80, 2, 78, TFT_getfontheight()+2, _bg);

    // time(&time_now);
	// time_last = 1554253843;
    gettimeofday(&tv, &tz);
	tm_info = localtime((time_t*)&tv.tv_sec);
 
    sprintf(tmp_buff, "%02d:%02d:%02d ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    TFT_print(tmp_buff, RIGHT, 5);
 
    _bg = TFT_BLACK;
    cfont = curr_font;
}
//==================================================================================
static int Wait(int ms)
{
	uint8_t tm = 1;
	if (ms < 0) {
		tm = 0;
		ms *= -1;
	}
	if (ms <= 50) {
		vTaskDelay(ms / portTICK_RATE_MS);
	}
	else {
		for (int n=0; n<ms; n += 50) {
			vTaskDelay(50 / portTICK_RATE_MS);
			// if (tm) _checkTime();
		}
	}
	return 1;
}
//-------------------------------------------------------------------

static void display_clock_task(void* p)
{
    menu_state_event_t state = {
        .state = DISPLAY_TIME,
    };
    while(1)
    {
        gettimeofday(&tv, &tz);
        tm_info = localtime((time_t*)&tv.tv_sec);
        // if(tm_info->tm_sec == 0)
            xQueueSend(xMenuQueue, &state, 0);
        
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
//-------------------menu items callbacks---------------------------//
// static menu_state_event_t menu_state = {0};
static bool smartconfig = false;
static bool ble_prov = false;

static void enter_main_menu(void* arg, uint8_t btn)
{
    menu_state_event_t state = {
        .state = MAIN_MENU
    };

    if(xQueueSend(xMenuQueue, &state, 0) != pdTRUE)
        ESP_LOGE(TAG, "cant init");
}

static void start_smartconfig_task(void* p)
{
    smartconfig = true;
    if(WIFI_Provision() != eWiFiSuccess)
        ESP_LOGE(TAG, "error to start smartconfig");

    smartconfig = false;    
    vTaskDelete(NULL);
}

static void stop_smartconfig(void* arg, uint8_t btn)
{
    system_event_t evt;
    evt.event_id = SYSTEM_EVENT_STA_DISCONNECTED;
    esp_event_send(&evt);
}

static void start_smartconfig(void* arg, uint8_t btn)
{
    xTaskCreate(start_smartconfig_task, "smartT", 2*1024, NULL, 2, NULL);
}

static void enter_smartconfig_menu(void* arg, uint8_t btn)
{
    menu_state_event_t state = {
        .state = SMARTCONFIG_MENU,
    };
    xQueueSend(xMenuQueue, &state, 0);
}

static void stop_ble_advertising(void* arg, uint8_t btn)
{
    IotBle_StopAdv();
    ble_prov = false;
}

static void start_ble_advertising(void* arg, uint8_t btn)
{
    BTStatus_t xStatus = IotBle_StartAdv(NULL);
    ble_prov = true;
}

static void enter_ble_provisioning_menu(void* arg, uint8_t btn)
{
    menu_state_event_t state = {
        .state = BLE_PROVISION_MENU,
    };
    xQueueSend(xMenuQueue, &state, 0);
}

static void confirm_pin_yes(void* arg, uint8_t btn)
{
    BaseType_t confirm = pdTRUE;
    xQueueSend(xBlePinConfirm, &confirm, 0);
}

static void confirm_pin_no(void* arg, uint8_t btn)
{
    BaseType_t confirm = pdFALSE;
    xQueueSend(xBlePinConfirm, &confirm, 0);
}

static void enter_wifi_scan_menu(void* arg, uint8_t btn)
{
    menu_state_event_t state = {
        .state = WIFI_SCAN_CONNECT_MENU,
    };
    xQueueSend(xMenuQueue, &state, 0);
}

static void start_wifi_scan(void* arg, uint8_t btn)
{
    menu_state_event_t state = {
        .state = WIFI_SCAN_MENU,
    };
    xQueueSend(xMenuQueue, &state, 0);
}
static void ap_station_info(void* arg, uint8_t btn)
{
    menu_state_event_t state = {
        .state = WIFI_AP_INFO_MENU,
        .params.ap_info.station = arg
    };
    // memcpy(&state.params.ap_info.station, arg, sizeof(WIFIScanResult_t*));

    xQueueSend(xMenuQueue, &state, 0);
}

static void connect_with_wps(void* arg, uint8_t btn)
{
    
}
//-------------------menu items callbacks----------------------//
//-------------------menu items--------------------------------//

//----smartconfig provisioning----//
menu_item_t provision_smartconfig_menu_item = {
	.callbacks = {NULL, enter_smartconfig_menu, next_menu},
	.buttons = {NULL, "SELECT", ">"},
	.title = "SmartConfig",
    .arg = (int*)1
};

menu_item_t start_smartconfig_menu_item = {
	.callbacks = {enter_main_menu, start_smartconfig, next_menu},
	.buttons = {"BACK", "SELECT", ">"},
	.title = "Start smartconfig",
    .arg = 1
};

menu_item_t stop_smartconfig_menu_item = {
	.callbacks = {enter_main_menu, stop_smartconfig, next_menu},
	.buttons = {"BACK", "SELECT", ">"},
	.title = "Stop smartconfig",
    .arg = 1
};

//----BLE provisioning----//
menu_item_t provision_ble_menu_item = {
	.callbacks = {NULL, enter_ble_provisioning_menu, next_menu},
	.buttons = {NULL, "SELECT", ">"},
	.title = "BLE provisioning",
    .arg = 1
};

menu_item_t start_ble_menu_item = {
	.callbacks = {enter_main_menu, start_ble_advertising, next_menu},
	.buttons = {"BACK", "SELECT", ">"},
	.title = "Start smartconfig",
    .arg = 1
};

menu_item_t stop_ble_menu_item = {
	.callbacks = {enter_main_menu, stop_ble_advertising, next_menu},
	.buttons = {"BACK", "SELECT", ">"},
	.title = "Stop smartconfig",
    .arg = 1
};

menu_item_t ble_confirm_pin_menu_item = {
	.callbacks = {enter_ble_provisioning_menu, confirm_pin_yes, confirm_pin_no},
	.buttons = {"BACK", "YES", "NO"},
	.title = "TODO"
};

//----WIFI scan & WPS----//
menu_item_t wifi_menu_item = {
	.callbacks = {NULL, enter_wifi_scan_menu, next_menu},
	.buttons = {NULL, "SELECT", ">"},
	.title = "WiFi scan & connect",
};

menu_item_t wifi_start_scan_menu_item = {
	.callbacks = {enter_main_menu, start_wifi_scan, next_menu},
	.buttons = {"BACK", "SELECT", ">"},
	.title = "Start wifi scan",
};

menu_item_t wifi_connect_wps_menu_item = {
	.callbacks = {enter_main_menu, connect_with_wps, next_menu},
	.buttons = {"BACK", "SELECT", ">"},
	.title = "Connect with WPS",
};

//-------------------menu items--------------------------------//


//****************************************************************//  DEMO ONLY
static WIFIScanResult_t scan_results[MAX_MENUS] = {0};

menu_item_t dummy_item = {
	.callbacks = {enter_main_menu, enter_main_menu, next_menu},
	.buttons = {"BACK", "TODO", ">"},
	.title = "TODO",
    .arg = 1
};

static void try_to_connect_ap(void* arg, uint8_t btn)
{
    WIFIScanResult_t* result = (WIFIScanResult_t*)arg;
    const WIFINetworkParams_t pxNetworkParams = {
        .pcSSID = (char*)result,
        .ucSSIDLength = strlen((char*)result)+1,
        // .pcPassword = pass,
        // .ucPasswordLength = strlen(pass)+1,
        .xSecurity = result->xSecurity
    };

    WIFIReturnCode_t err = WIFI_ConnectAP(&pxNetworkParams);
    if(err != eWiFiSuccess)
        ESP_LOGE(TAG, "fail to connect %s, %d", (char*)&pxNetworkParams, err);

    enter_main_menu(NULL, NULL);

}

//***********************************************************************// DEMO ONLY

//===============
void tft_demo() {
	TFT_resetclipwin();
    TFT_fillScreen(_bg);

	TFT_setFont(COMIC24_FONT, NULL);

    disp_header(PROJ_NAME);
    ESP_LOGI(TAG, "start loop");
    xTaskCreate(display_clock_task, "clockT", 1*1024, NULL, 0, NULL);
    menu_state_event_t init = {
        .state = MAIN_MENU,
    };

    if(xQueueSend(xMenuQueue, &init, 0) != pdTRUE)
        ESP_LOGE(TAG, "cant init");

    init.state = DISPLAY_TIME;
    if(xQueueSend(xMenuQueue, &init, 0) != pdTRUE)
        ESP_LOGE(TAG, "cant init");
	while (1) 
    {
        if(tft_callback_task())
        {
            // todo reset display off timer
        }

        // TFT_fillCircle(13, 10, 8, TFT_BLUE);
        TFT_resetclipwin();

        if(xNetworkState == eNetworkStateEnabled)
            TFT_fillCircle(33, 10, 8, TFT_GREEN);
        else
            TFT_fillCircle(33, 10, 8, TFT_RED);

        if(smartconfig)
            TFT_fillCircle(53, 10, 8, TFT_PINK);
        else
            TFT_fillCircle(53, 10, 8, TFT_RED);
        
        if(ble_prov)
            TFT_fillCircle(73, 10, 8, TFT_BLUE);
        else
            TFT_fillCircle(73, 10, 8, TFT_RED);
        static menu_item_t* wifi_ap_table[MAX_MENUS] = {NULL};

        menu_state_event_t pvMenuState;
        if(xQueueReceive(xMenuQueue, &pvMenuState, 10/portTICK_PERIOD_MS) == pdTRUE)
        {
            // ESP_LOGI(TAG, "state: %d", pvMenuState.state);
            switch(pvMenuState.state)
            {
                case MAIN_MENU:
                {
                    clear_menu();
                    menu_add_item(&provision_smartconfig_menu_item);
                    menu_add_item(&provision_ble_menu_item);
                    menu_add_item(&wifi_menu_item);
                    draw_menu();
                }
                break;
                case SMARTCONFIG_MENU:
                {
                    clear_menu();
                    menu_add_item(&start_smartconfig_menu_item);
                    menu_add_item(&stop_smartconfig_menu_item);
                    draw_menu();
                }
                break;
                case BLE_PROVISION_MENU:
                {
                    clear_menu();
                    menu_add_item(&start_ble_menu_item);
                    menu_add_item(&stop_ble_menu_item);
                    draw_menu();
                }
                break;
                case WIFI_SCAN_CONNECT_MENU:
                {
                    clear_menu();
                    menu_add_item(&wifi_start_scan_menu_item);
                    menu_add_item(&wifi_connect_wps_menu_item);
                    draw_menu();
                }
                break;
                case DISPLAY_TIME:
                {
                    _dispTime();
                }
                break;
                case WIFI_AP_INFO_MENU:
                {
                    WIFIScanResult_t record = *pvMenuState.params.ap_info.station;
                    menu_item_t _dummy_item = {
                        .callbacks = {enter_main_menu, NULL, try_to_connect_ap},
                        .buttons = {"BACK", NULL, "CONNECT"},
                        .title = NULL,
                        .arg = pvMenuState.params.ap_info.station
                    };

                    char router_mac[25];
                    char ssid[33];
                    TFT_saveClipWin();
                    TFT_setclipwin(15, 39, _width-20, _height);
                    TFT_fillWindow(_bg);
                    clear_menu();
                    menu_add_item(&_dummy_item);
                    draw_menu();

                    sprintf(router_mac, "%02X:%02X:%02X:%02X:%02X:%02X   ", record.ucBSSID[0], record.ucBSSID[1], record.ucBSSID[2], record.ucBSSID[3], record.ucBSSID[4], record.ucBSSID[5]);
                    sprintf(ssid, "%s   ", record.cSSID);
                    TFT_print("SSID", 0, TFT_getfontheight()+2); TFT_print(ssid, RIGHT, LASTY);
                    TFT_print("AP MAC", 0, LASTY + TFT_getfontheight()+2); TFT_print(router_mac, RIGHT, LASTY);
                    TFT_print("Security", 0, LASTY + TFT_getfontheight()+2); 
                    if(record.xSecurity == eWiFiSecurityOpen)
                        TFT_print("Open   ", RIGHT, LASTY);
                    else if(record.xSecurity == eWiFiSecurityWEP)
                        TFT_print("WEP   ", RIGHT, LASTY);
                    else if(record.xSecurity == eWiFiSecurityWPA)
                        TFT_print("WPA   ", RIGHT, LASTY);
                    else if(record.xSecurity == eWiFiSecurityWPA2)
                        TFT_print("WPA/WPA2 PSK   ", RIGHT, LASTY);

                    TFT_restoreClipWin();
                }
                break;
                case WIFI_SCAN_MENU:
                {
                    TFT_saveClipWin();
                    TFT_setclipwin(0, 39, _width, _height);
                    TFT_fillWindow(_bg);

                    clear_menu();
                    TFT_print("Scanning ...", CENTER, CENTER);

                    WIFIReturnCode_t ret = WIFI_Scan(&scan_results, MAX_MENUS);
                    TFT_fillWindow(_bg);
                    if(ret == eWiFiSuccess)
                    {
                        for(int i=0;i<MAX_MENUS;i++)
                        {
                            if(wifi_ap_table[i] != NULL)
                            {
                                if(wifi_ap_table[i]->title != NULL)
                                    free(wifi_ap_table[i]->title);

                                free(wifi_ap_table[i]);
                            }
                            wifi_ap_table[i] = NULL;
                            WIFIScanResult_t* tmp = &scan_results[i];

                            if(strcmp((char*)tmp->cSSID, "") == 0)
                                continue;

                            // ESP_LOGI(TAG, "ssid: %s", (char*)tmp->cSSID);
                            menu_item_t* wifi_station_item = (menu_item_t*)calloc(1, sizeof(menu_item_t));
                            wifi_station_item->title = (char*)calloc(33, 1);
                            strcpy(wifi_station_item->title, (char*)tmp->cSSID);
                            wifi_station_item->callbacks[0] = enter_wifi_scan_menu;
                            wifi_station_item->callbacks[1] = ap_station_info;
                            wifi_station_item->callbacks[2] = next_menu;
                            wifi_station_item->buttons[0] = "BACK";
                            wifi_station_item->buttons[1] = "SELECT";
                            wifi_station_item->buttons[2] = ">";
                            wifi_station_item->arg = tmp;

                            wifi_ap_table[i] = wifi_station_item;

                            menu_add_item(wifi_station_item);
                        }
                    }
                    draw_menu();
                    TFT_restoreClipWin();

                    heap_caps_print_heap_info(0);
                
                }
                break;
                case BLE_PIN_COMPARISION:
                {
                    char pin[20];
                    sprintf(pin, "PIN: %d", pvMenuState.params.ble_pin.pin);
                    clear_menu();
                    ble_confirm_pin_menu_item.title = pin;
                    menu_add_item(&ble_confirm_pin_menu_item);
                    draw_menu();
                }
                break;
                default:
                break;
            }
        }        
	}
}




//=============
void init_tft(void* p)
{
    // init_sd_card();
    // ========  PREPARE DISPLAY INITIALIZATION  =========
    // esp_err_t ret;
    
    tv.tv_sec = 1554264193;
    tz.tz_minuteswest = 16*60;
    settimeofday(&tv, &tz);

    start_spi_display();
	TFT_resetclipwin();
    obtain_time();
    display_splash_screen(TFT_WHITE);

    // xWiFiStationsSearch = xQueueCreate(2, sizeof(WIFIScanResult_t));
    xMenuQueue = xQueueCreate(15, sizeof(menu_state_event_t));
    xBlePinConfirm = xQueueCreate(1, sizeof(BaseType_t));

	Wait(-2000);

    if(init_menu() != ESP_OK)
        disp_header("FAILED");

    while((AwsIotNetworkManager_GetConfiguredNetworks() & configENABLED_NETWORKS) == 0)
        Wait(50);

    static SubscriptionHandle_t xHandle = NULL;
    BaseType_t _ret;
    do{
        _ret = AwsIotNetworkManager_SubscribeForStateChange(AWSIOT_NETWORK_TYPE_WIFI, wifiStateCallback, NULL, &xHandle);
        Wait(50);
    }while(_ret != pdTRUE);

	//=========
    // Run demo
    //=========
	tft_demo();
}

static void wifiStateCallback(uint32_t ulNetworkType, AwsIotNetworkState_t _xNetworkState, void* pvContext)
{
    xNetworkState = _xNetworkState;
}

// ================== TEST SD CARD ==========================================

#define sdPIN_NUM_MISO PIN_NUM_MISO
#define sdPIN_NUM_MOSI PIN_NUM_MOSI
#define sdPIN_NUM_CLK  PIN_NUM_CLK
#define sdPIN_NUM_CS   4

static void init_sd_card(void)
{
#ifdef SD_CARD_DEMO

    printf("\n=======================================================\n");
    printf("===== Test using SD Card in SPI mode              =====\n");
    printf("===== SD Card uses the same gpio's as TFT display =====\n");
    printf("=======================================================\n\n");
    ESP_LOGI(TAG, "Initializing SD card");
    ESP_LOGI(TAG, "Using SPI peripheral");
    // host = SDSPI_HOST_DEFAULT();
    // slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    host.slot = VSPI_HOST;
    slot_config.gpio_miso = sdPIN_NUM_MISO;
    slot_config.gpio_mosi = sdPIN_NUM_MOSI;
    slot_config.gpio_sck  = sdPIN_NUM_CLK;
    slot_config.gpio_cs   = sdPIN_NUM_CS;
    slot_config.dma_channel = 2;

    // host.set_card_clk();

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%d). "
                "Make sure SD card lines have pull-up resistors in place.", ret);
        }
        return;
    }
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
                ESP_LOGI(TAG, "Opening file");
                FILE* f = fopen("/sdcard/hello.txt", "w");
                if (f == NULL) {
                    ESP_LOGE(TAG, "Failed to open file for writing");
                    goto exit;
                }
                fprintf(f, "Hello %s!\n", card->cid.name);
                fclose(f);
                ESP_LOGI(TAG, "File written");
                // Check if destination file exists before renaming
                struct stat st;
                if (stat("/sdcard/foo.txt", &st) == 0) {
                    // Delete it if it exists
                    // unlink("/sdcard/foo.txt");
                }
                // Rename original file
                ESP_LOGI(TAG, "Renaming file");
                if (rename("/sdcard/hello.txt", "/sdcard/foo.txt") != 0) {
                    ESP_LOGE(TAG, "Rename failed");
                    // return;
                }
                // Open renamed file for reading
                ESP_LOGI(TAG, "Reading file");
                f = fopen("/sdcard/foo.txt", "r");
                if (f == NULL) {
                    ESP_LOGE(TAG, "Failed to open file for reading");
                    return;
                }
                char line[64];
                fgets(line, sizeof(line), f);
                fclose(f);
                // strip newline
                char* pos = strchr(line, '\n');
                if (pos) {
                    *pos = '\0';
                }
                ESP_LOGI(TAG, "Read from file: '%s'", line);
                // All done, unmount partition and disable SDMMC or SPI peripheral
                exit:
                esp_vfs_fat_sdmmc_unmount();
                ESP_LOGI(TAG, "Card unmounted");
                printf("===== SD Card test end ================================\n\n");
#endif                
}
// ================== TEST SD CARD ==========================================

static void start_spi_display()
{
    esp_err_t ret;
    spi_lobo_device_handle_t spi;
    tft_disp_type = DEFAULT_DISP_TYPE;
	_width = DEFAULT_TFT_DISPLAY_WIDTH;  // smaller dimension
	_height = DEFAULT_TFT_DISPLAY_HEIGHT; // larger dimension
	//_width = 128;  // smaller dimension
	//_height = 160; // larger dimension
	// ===================================================

	// ===================================================
	// ==== Set maximum spi clock for display read    ====
	//      operations, function 'find_rd_speed()'    ====
	//      can be used after display initialization  ====
	max_rdclock = 8000000;
	// ===================================================

    // ====================================================================
    // === Pins MUST be initialized before SPI interface initialization ===
    // ====================================================================
    TFT_PinsInit();

    // ====  CONFIGURE SPI DEVICES(s)  ====================================================================================

	
    spi_lobo_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,				// set SPI MISO pin
        .mosi_io_num=PIN_NUM_MOSI,				// set SPI MOSI pin
        .sclk_io_num=PIN_NUM_CLK,				// set SPI CLK pin
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
		.max_transfer_sz = 6*1024,
    };
    spi_lobo_device_interface_config_t devcfg={
        .clock_speed_hz=8000000,                // Initial clock out at 8 MHz
        .mode=0,                                // SPI mode 0
        .spics_io_num=-1,                       // we will use external CS pin
		.spics_ext_io_num=PIN_NUM_CS,           // external CS pin
		.flags=LB_SPI_DEVICE_HALFDUPLEX,        // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
    };


    // ====================================================================================================================


    vTaskDelay(500 / portTICK_RATE_MS);
	printf("\r\n==============================\r\n");
    printf("TFT display DEMO, LoBo 11/2017\r\n");
	printf("==============================\r\n");
    printf("Pins used: miso=%d, mosi=%d, sck=%d, cs=%d\r\n", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
	printf("==============================\r\n\r\n");

	// ==================================================================
	// ==== Initialize the SPI bus and attach the LCD to the SPI bus ====

	ret=spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    assert(ret==ESP_OK);
	printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
	disp_spi = spi;

	// ==== Test select/deselect ====
	ret = spi_lobo_device_select(spi, 1);
    assert(ret==ESP_OK);
	ret = spi_lobo_device_deselect(spi);
    assert(ret==ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(spi));
	printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(spi) ? "true" : "false");

	// ================================
	// ==== Initialize the Display ====

	printf("SPI: display init...\r\n");
	TFT_display_init();
    printf("OK\r\n");
	
	// ---- Detect maximum read speed ----
	max_rdclock = find_rd_speed();
	printf("SPI: Max rd speed = %u\r\n", max_rdclock);

    // ==== Set SPI clock used for display operations ====
	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
	printf("SPI: Changed speed to %u\r\n", spi_lobo_get_speed(spi));

    printf("\r\n---------------------\r\n");
	printf("Graphics demo started\r\n");
	printf("---------------------\r\n");

	font_rotate = 0;
	text_wrap = 1;
	font_transparent = 0;
	font_forceFixed = 0;
	gray_scale = 0;
    TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(LANDSCAPE);
	// TFT_setFont(DEFAULT_FONT, NULL);
}

// -----------------------------------------------------//

extern void tft_display_ble(void* p)
{

}

// ---------------------------------------------------//
extern void _BLENumericComparisonCb(BTBdaddr_t * pxRemoteBdAddr, uint32_t ulPassKey)
{
    menu_state_event_t pin = {
        .state = BLE_PIN_COMPARISION,
        .params.ble_pin.pin = ulPassKey
    };

    xQueueSend(xMenuQueue, &pin, 10);
}

#include "aws_ble_numericComparison.h"
BaseType_t getUserMessage( INPUTMessage_t * pxINPUTmessage, TickType_t xAuthTimeout )
{
	BaseType_t xEvent = pdFALSE;
	BaseType_t xReturnMessage = pdFALSE;

	if (xQueueReceive(xBlePinConfirm, (void * )&xEvent, (portTickType) xAuthTimeout )) {
        pxINPUTmessage->pcData = (uint8_t *)malloc(1);
        if(pxINPUTmessage->pcData != NULL){
            memset(pxINPUTmessage->pcData,0x0,1);
            if(xEvent == pdTRUE)
                *pxINPUTmessage->pcData = 'y';
            else
            {
                *pxINPUTmessage->pcData = 'n';
            }
            
            xReturnMessage = pdTRUE;
        }else
        {
            configPRINTF(("Malloc failed in main.c\n"));
        }
	}

	return xReturnMessage;
}
