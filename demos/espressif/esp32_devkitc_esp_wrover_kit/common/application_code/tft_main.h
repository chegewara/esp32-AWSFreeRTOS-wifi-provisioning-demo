#ifndef __TFT_MAIN_H__
#define __TFT_MAIN_H__


typedef enum{
    MAIN_MENU,
    SMARTCONFIG_MENU,
    BLE_PROVISION_MENU,
    WIFI_SCAN_CONNECT_MENU,
    WIFI_SCAN_MENU,
    WIFI_AP_INFO_MENU,
    DISPLAY_TIME,
    BLE_PIN_COMPARISION,
    MAX_MENU
}state_menu_t;

typedef union{
    struct menu_ap_info_evt_param{
        WIFIScanResult_t* station;
    }ap_info;

    struct menu_ble_pin_comparision_evt_param{
        uint32_t pin;
    }ble_pin;
}state_menu_event_t;

typedef struct{
	state_menu_t state;
    state_menu_event_t params;
}menu_state_event_t;

#endif
