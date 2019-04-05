/**
 * @file aws_ble_hal_gap.c
 * @brief Hardware Abstraction Layer for GAP ble stack.
 */

#include <stddef.h>
#include <string.h>
#include "FreeRTOS.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_common_api.h"
#include "bt_hal_manager_adapter_ble.h"
#include "bt_hal_manager.h"
#include "bt_hal_gatt_client.h"
// #include "bt_hal_gatt_server.h"
#include "aws_ble_hal_internals.h"
#include "esp_log.h"

static uint8_t APP_ID = 1; // needs to be different than server APP_ID

static BTGattClientCallbacks_t xGattClientCb;
static bool bInterfaceCreated = false;
static uint32_t ulGattClientIFhandle;
static uint32_t ulGattConnIdhandle;
BTGattDbElement_t* xGattDB;
static BTUuid_t xClientUuidCb;

/** Register a client application */
static BTStatus_t prvBTRegisterClient( BTUuid_t * pxUuid );

/** Unregister a client application from the stack */
static BTStatus_t prvBTUnregisterClient( uint8_t ucClientIf );

/**
 * Initializes the interface and provides callback routines
 */
static BTStatus_t prvBTGattClientInit( const BTGattClientCallbacks_t * pxCallbacks );

/** Create a connection to a remote LE or dual-mode device */
static BTStatus_t prvBTConnect( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr, bool bIsDirect, BTTransport_t xTransport );

/** Disconnect a remote device or cancel a pending connection */
static BTStatus_t prvBTDisconnect( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr, uint16_t usConnId );

/** Clear the attribute cache for a given device */
static BTStatus_t prvBTRefresh( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr );

/**
 * Enumerate all GATT services on a connected device.
 * Optionally, the results can be filtered for a given UUID.
 */
static BTStatus_t prvBTSearchService( uint16_t usConnId, BTUuid_t * pxFilterUuid );

/** Read a characteristic on a remote device */
/*!!TODO Types for auth request */
static BTStatus_t prvBTReadCharacteristic( uint16_t usConnId, uint16_t usHandle, uint32_t ulAuthReq );

/** Write a remote characteristic */
/*!!TODO Types for auth request */
static BTStatus_t prvBTWriteCharacteristic( uint16_t usConnId, uint16_t usHandle, BTAttrWriteRequests_t xWriteType, 
                                        size_t xLen, uint32_t ulAuthReq, char * pcValue );

/** Read the descriptor for a given characteristic */
/*!!TODO Types for auth request */
static BTStatus_t prvBTReadDescriptor( uint16_t usConnId, uint16_t usHandle, uint32_t ulAuthReq );

/** Write a remote descriptor for a given characteristic */
static BTStatus_t prvBTWriteDescriptor( uint16_t usConnId, uint16_t usHandle, BTAttrWriteRequests_t xWriteType,
                                        size_t xLen, uint32_t ulAuthReq, char * pcValue );

/** Execute a prepared write operation */
static BTStatus_t prvBTExecuteWrite( uint16_t usConnId, bool bExecute );

/**
 * Register to receive notifications or indications for a given
 * characteristic
 */
static BTStatus_t prvBTRegisterForNotification( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr, uint16_t usHandle );

/** Deregister a previous request for notifications/indications */
static BTStatus_t prvBTUnregisterForNotification( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr, uint16_t usHandle );

/** Request RSSI for a given remote device */
static BTStatus_t prvBTReadRemoteRssi( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr );

/** Determine the type of the remote device (LE, BR/EDR, Dual-mode) */
static BTTransport_t prvBTGetDeviceType( const BTBdaddr_t * pxBdAddr );

/** Configure the MTU for a given connection */
static BTStatus_t prvBTConfigureMtu( uint16_t usConnId, uint16_t usMtu );

/** Test mode interface */
static BTStatus_t prvBTTestCommand ( uint32_t ulCommand, BTGattTestParams_t * pxParams );

/** Get gatt db content */
static BTStatus_t prvBTGetGattDb ( uint16_t usConnId, uint16_t* count );

BTGattClientInterface_t xGATTclientInterface = {
    .pxRegisterClient = prvBTRegisterClient,
    .pxUnregisterClient = prvBTUnregisterClient,
    .pxGattClientInit = prvBTGattClientInit,
    .pxConnect = prvBTConnect,
    .pxDisconnect = prvBTDisconnect,
    .pxRefresh = prvBTRefresh,
    .pxSearchService = prvBTSearchService,
    .pxReadCharacteristic = prvBTReadCharacteristic,
    .pxWriteCharacteristic = prvBTWriteCharacteristic,
    .pxReadDescriptor = prvBTReadDescriptor,
    .pxWriteDescriptor = prvBTWriteDescriptor,
    .pxExecuteWrite = prvBTExecuteWrite,
    .pxRegisterForNotification = prvBTRegisterForNotification,
    .pxUnregisterForNotification = prvBTUnregisterForNotification,
    .pxReadRemoteRssi = prvBTReadRemoteRssi,
    .pxGetDeviceType = prvBTGetDeviceType,
    .pxConfigureMtu = prvBTConfigureMtu,
    .pxTestCommand = prvBTTestCommand,
    .pxGetGattDb = prvBTGetGattDb
};



void prvGATTCeventHandler( esp_gattc_cb_event_t event, 
                            esp_gatt_if_t gattc_if, 
                            esp_ble_gattc_cb_param_t *param)
{
    switch( event )
    {
        case ESP_GATTC_REG_EVT:
            if(APP_ID != param->reg.app_id)
                break;
			ulGattClientIFhandle = gattc_if;
            if(xGattClientCb.pxRegisterClientCb != NULL)
            {
                xGattClientCb.pxRegisterClientCb(param->reg.status, gattc_if, &xClientUuidCb);
            }
            break;
        case ESP_GATTC_UNREG_EVT:
            ESP_LOG_BUFFER_HEX(__func__, param, 16);
            break;
        case ESP_GATTC_OPEN_EVT:
            ulGattConnIdhandle = param->open.conn_id;
            if(xGattClientCb.pxOpenCb != NULL)
            {
                xGattClientCb.pxOpenCb(param->open.conn_id, param->open.status, gattc_if, (BTBdaddr_t*)param->open.remote_bda);
            }
            break;
        case ESP_GATTC_READ_CHAR_EVT:
            if(xGattClientCb.pxReadCharacteristicCb != NULL)
            {
                BTGattUnformattedValue_t value = {
                    .usLen = param->read.value_len
                };
                memcpy(value.ucValue, param->read.value, param->read.value_len);
                BTGattReadParams_t data = {
                    .usHandle = param->read.handle,
                    .ucStatus = param->read.status,
                    .usValueType = NULL,
                    .xValue = value
                };
                xGattClientCb.pxReadCharacteristicCb(param->read.conn_id, param->read.status, &data);
            }
            break;
        break;
        case ESP_GATTC_WRITE_CHAR_EVT:
            if(xGattClientCb.pxWriteCharacteristicCb != NULL)
            {
                xGattClientCb.pxWriteCharacteristicCb(param->write.conn_id, param->write.status, param->write.handle);
            }
            break;
        case ESP_GATTC_CLOSE_EVT:
            if(xGattClientCb.pxCloseCb != NULL)
            {
                xGattClientCb.pxCloseCb(param->close.conn_id, param->close.status, gattc_if, (BTBdaddr_t*)param->close.remote_bda);
            }
            break;
        case ESP_GATTC_SEARCH_CMPL_EVT:
            if(xGattClientCb.pxSearchCompleteCb != NULL)
            {
                xGattClientCb.pxSearchCompleteCb(param->search_cmpl.conn_id, param->search_cmpl.status);
            }
            break;
        case ESP_GATTC_SEARCH_RES_EVT:
            break;
        case ESP_GATTC_READ_DESCR_EVT:
            if(xGattClientCb.pxReadDescriptorCb != NULL)
            {
                BTGattUnformattedValue_t value = {
                    .usLen = param->read.value_len
                };
                memcpy(value.ucValue, param->read.value, param->read.value_len);
                BTGattReadParams_t data = {
                    .usHandle = param->read.handle,
                    .ucStatus = param->read.status,
                    .usValueType = NULL,
                    .xValue = value
                };
                xGattClientCb.pxReadDescriptorCb(param->read.conn_id, param->read.status, &data);
            }
            break;
        case ESP_GATTC_WRITE_DESCR_EVT:
            if(xGattClientCb.pxWriteDescriptorCb != NULL)
            {
                xGattClientCb.pxWriteDescriptorCb(param->write.conn_id, param->write.status, param->write.handle);
            }
            break;
        case ESP_GATTC_NOTIFY_EVT:
            if(xGattClientCb.pxNotifyCb != NULL)
            {
                BTGattNotifyParams_t pxData = {
                    .usHandle = param->notify.handle,
                    .xLen = param->notify.value_len,
                    .bIsNotify = param->notify.is_notify
                };
                memcpy(pxData.xBda.ucAddress, param->notify.remote_bda, 6);
                memcpy(pxData.ucValue, param->notify.value, pxData.xLen);
                xGattClientCb.pxNotifyCb(param->notify.conn_id, &pxData);
            }
            break;
        case ESP_GATTC_PREP_WRITE_EVT:
            break;
        case ESP_GATTC_EXEC_EVT:
            if(xGattClientCb.pxExecuteWriteCb != NULL)
            {
                xGattClientCb.pxExecuteWriteCb(param->exec_cmpl.conn_id, param->exec_cmpl.status);
            }
            break;
        case ESP_GATTC_ACL_EVT:
            break;
        case ESP_GATTC_CANCEL_OPEN_EVT:
            break;
        case ESP_GATTC_SRVC_CHG_EVT:
            break;
        case ESP_GATTC_ENC_CMPL_CB_EVT:
            break;
        case ESP_GATTC_CFG_MTU_EVT:
            if(xGattClientCb.pxConfigureMtuCb != NULL)
            {
                xGattClientCb.pxConfigureMtuCb(param->cfg_mtu.conn_id, param->cfg_mtu.status, param->cfg_mtu.mtu);
            }
            break;
        case ESP_GATTC_ADV_DATA_EVT:
        break;
        case ESP_GATTC_MULT_ADV_ENB_EVT:
        break;
        case ESP_GATTC_MULT_ADV_UPD_EVT:
        break;
        case ESP_GATTC_MULT_ADV_DATA_EVT:
        break;
        case ESP_GATTC_MULT_ADV_DIS_EVT:
        break;
        case ESP_GATTC_CONGEST_EVT:
            if(xGattClientCb.pxCongestionCb != NULL)
            {
                xGattClientCb.pxCongestionCb(param->congest.conn_id, param->congest.congested);
            }
            break;
        case ESP_GATTC_BTH_SCAN_ENB_EVT:
        break;
        case ESP_GATTC_BTH_SCAN_CFG_EVT:
        break;
        case ESP_GATTC_BTH_SCAN_RD_EVT:
        break;
        case ESP_GATTC_BTH_SCAN_THR_EVT:
        break;
        case ESP_GATTC_BTH_SCAN_PARAM_EVT:
        break;
        case ESP_GATTC_BTH_SCAN_DIS_EVT:
        break;
        case ESP_GATTC_SCAN_FLT_CFG_EVT:
        break;
        case ESP_GATTC_SCAN_FLT_PARAM_EVT:
        break;
        case ESP_GATTC_SCAN_FLT_STATUS_EVT:
        break;
        case ESP_GATTC_ADV_VSC_EVT:
        break;
        case ESP_GATTC_REG_FOR_NOTIFY_EVT:
            if(xGattClientCb.pxRegisterForNotificationCb != NULL){
                xGattClientCb.pxRegisterForNotificationCb(0, true,  param->reg_for_notify.status, param->reg_for_notify.handle);
            }
            break;
        case ESP_GATTC_UNREG_FOR_NOTIFY_EVT:
            if(xGattClientCb.pxRegisterForNotificationCb != NULL){
                xGattClientCb.pxRegisterForNotificationCb(0, false,  param->reg_for_notify.status, param->reg_for_notify.handle);
            }
            break;
        case ESP_GATTC_CONNECT_EVT:
        break;
        case ESP_GATTC_DISCONNECT_EVT:
            if(xGattClientCb.pxCloseCb != NULL)
            {
                xGattClientCb.pxCloseCb(param->disconnect.conn_id, param->disconnect.reason, gattc_if, (BTBdaddr_t*)param->disconnect.remote_bda );
            }
            break;
        case ESP_GATTC_READ_MULTIPLE_EVT:
        break;
        case ESP_GATTC_QUEUE_FULL_EVT:
        break;
        case ESP_GATTC_SET_ASSOC_EVT:
        break;
        case ESP_GATTC_GET_ADDR_LIST_EVT:
        break;
        
        default:
            printf("unknown gattc event");
            break;
    }

}

BTStatus_t prvBTRegisterClient( BTUuid_t * pxUuid )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    if( bInterfaceCreated == false )
    {

        if( esp_ble_gattc_app_register( APP_ID ) != ESP_OK )
        {
            xStatus = eBTStatusFail;
        }
    }
    else
    {
        xGattClientCb.pxRegisterClientCb( eBTStatusSuccess, ulGattClientIFhandle, NULL );
    }

    return xStatus;
}

/** Unregister a client application from the stack */
BTStatus_t prvBTUnregisterClient( uint8_t ucClientIf )
{
    BTStatus_t xStatus = eBTStatusUnsupported;
    xStatus = esp_ble_gattc_app_unregister(ucClientIf);
    return xStatus;
}

/**
 * Initializes the interface and provides callback routines
 */
BTStatus_t prvBTGattClientInit( const BTGattClientCallbacks_t * pxCallbacks )
{
    BTStatus_t xStatus = eBTStatusSuccess;


	if( pxCallbacks != NULL )
	{
		xGattClientCb = *pxCallbacks;
	}
	else
	{
		xStatus = eBTStatusParamInvalid;
	}

    if( xStatus == eBTStatusSuccess )
    {
        if( esp_ble_gattc_register_callback( prvGATTCeventHandler ) != ESP_OK )
        {
            xStatus = eBTStatusFail;
        }
    }

    return xStatus;
}

/** Create a connection to a remote LE or dual-mode device */
BTStatus_t prvBTConnect( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr, bool bIsDirect, BTTransport_t xTransport )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    esp_ble_addr_type_t bdaddr_type = BLE_ADDR_TYPE_PUBLIC;

    xStatus = esp_ble_gattc_open(ucClientIf, (uint8_t*)pxBdAddr->ucAddress, bdaddr_type, bIsDirect);
    // ulGattClientIFhandle = ucClientIf;

    return xStatus;
}

/** Disconnect a remote device or cancel a pending connection */
BTStatus_t prvBTDisconnect( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr, uint16_t usConnId )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    xStatus = esp_ble_gattc_close(ucClientIf, usConnId);

    return xStatus;
}

/** Clear the attribute cache for a given device */
BTStatus_t prvBTRefresh( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    xStatus = esp_ble_gattc_cache_refresh(&pxBdAddr->ucAddress);
    return xStatus;
}

/**
 * Enumerate all GATT services on a connected device.
 * Optionally, the results can be filtered for a given UUID.
 */
BTStatus_t prvBTSearchService( uint16_t usConnId, BTUuid_t * pxFilterUuid )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    if(pxFilterUuid != NULL){
        esp_bt_uuid_t pxUuid;
        prvCopytoESPUUID(&pxUuid, pxFilterUuid);
        xStatus = esp_ble_gattc_search_service(ulGattClientIFhandle, usConnId, &pxUuid);
    }
    else
    {
        xStatus = esp_ble_gattc_search_service(ulGattClientIFhandle, usConnId, NULL);
    }
    
    return xStatus;
}

/** Read a characteristic on a remote device */
/*!!TODO Types for auth request */
BTStatus_t prvBTReadCharacteristic( uint16_t usConnId, uint16_t usHandle, uint32_t ulAuthReq ) 
{
    BTStatus_t xStatus = eBTStatusSuccess;
    xStatus = esp_ble_gattc_read_char(ulGattClientIFhandle, usConnId, usHandle, ulAuthReq);
    return xStatus;
}

/** Write a remote characteristic */
/*!!TODO Types for auth request */
BTStatus_t prvBTWriteCharacteristic( uint16_t usConnId, uint16_t usHandle, BTAttrWriteRequests_t xWriteType, 
                                        size_t xLen, uint32_t ulAuthReq, char * pcValue ) 
{
    BTStatus_t xStatus = eBTStatusSuccess;

    xStatus = esp_ble_gattc_write_char(ulGattClientIFhandle, usConnId, usHandle, xLen, (uint8_t*)pcValue, xWriteType, ulAuthReq);

    return xStatus;
}

/** Read the descriptor for a given characteristic */
/*!!TODO Types for auth request */
BTStatus_t prvBTReadDescriptor( uint16_t usConnId, uint16_t usHandle, uint32_t ulAuthReq ) 
{
    BTStatus_t xStatus = eBTStatusSuccess;
    xStatus = esp_ble_gattc_read_char_descr(ulGattClientIFhandle, usConnId, usHandle, ulAuthReq);
    return xStatus;
}

/** Write a remote descriptor for a given characteristic */
BTStatus_t prvBTWriteDescriptor( uint16_t usConnId, uint16_t usHandle, BTAttrWriteRequests_t xWriteType,
                                        size_t xLen, uint32_t ulAuthReq, char * pcValue )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    xStatus = esp_ble_gattc_write_char_descr(ulGattClientIFhandle, usConnId, usHandle, xLen, (uint8_t*)pcValue, xWriteType, ulAuthReq);

    return xStatus;
}


/** Execute a prepared write operation */
BTStatus_t prvBTExecuteWrite( uint16_t usConnId, bool bExecute )
{
    BTStatus_t xStatus = eBTStatusSuccess;
    xStatus = esp_ble_gattc_execute_write(ulGattClientIFhandle, usConnId, bExecute);
    return xStatus;
}


/**
 * Register to receive notifications or indications for a given
 * characteristic
 */
BTStatus_t prvBTRegisterForNotification( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr, uint16_t usHandle )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    xStatus = esp_ble_gattc_register_for_notify(ulGattClientIFhandle, pxBdAddr->ucAddress, usHandle);

    return xStatus;
}


/** Deregister a previous request for notifications/indications */
BTStatus_t prvBTUnregisterForNotification( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr, uint16_t usHandle )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    xStatus = esp_ble_gattc_unregister_for_notify(ulGattClientIFhandle, pxBdAddr->ucAddress, usHandle-1);

    return xStatus;
}


/** Request RSSI for a given remote device */
BTStatus_t prvBTReadRemoteRssi( uint8_t ucClientIf, const BTBdaddr_t * pxBdAddr )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}


/** Determine the type of the remote device (LE, BR/EDR, Dual-mode) */
BTTransport_t prvBTGetDeviceType( const BTBdaddr_t * pxBdAddr )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}


/** Configure the MTU for a given connection */
BTStatus_t prvBTConfigureMtu( uint16_t usConnId, uint16_t usMtu )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    xStatus = esp_ble_gatt_set_local_mtu(usMtu);
    if(xStatus == ESP_OK)
        xStatus = esp_ble_gattc_send_mtu_req(ulGattClientIFhandle, usConnId);

    return xStatus;
}


/** Test mode interface */
BTStatus_t prvBTTestCommand ( uint32_t ulCommand, BTGattTestParams_t * pxParams )
{
    BTStatus_t xStatus = eBTStatusUnsupported;

    return xStatus;
}

/**
 * @brief function to convert esp attributes db to aws attributes db
 */
static BTAttribute_t espDBElToDBAttribute(esp_gattc_db_elem_t el)
{    
    BTUuid_t uuid;
    prvCopyFromESPUUI( &uuid, &el.uuid );
    esp_gatt_db_attr_type_t type = el.type;
    BTAttribute_t ret;
    BTCharPermissions_t perm = 0; // TODO

    switch(type)
    {
        case ESP_GATT_DB_PRIMARY_SERVICE:{
            ret.xAttributeType = eBTDbPrimaryService;
            ret.xServiceUUID = uuid;
            break;
        }
        case ESP_GATT_DB_SECONDARY_SERVICE:{
            ret.xAttributeType = eBTDbSecondaryService;
            ret.xServiceUUID = uuid;
            break;
        }
        case ESP_GATT_DB_CHARACTERISTIC:{
            BTCharacteristic_t xCharacteristic = {
                .xUuid = uuid,
                .xPermissions = perm,
                .xProperties = el.properties
            };
            ret.xAttributeType = eBTDbCharacteristic;
            ret.xCharacteristic = xCharacteristic;
            break;
        }
        case ESP_GATT_DB_DESCRIPTOR:{
            BTCharacteristicDescr_t xCharacteristicDescr = {
                .xUuid = uuid,
                .xPermissions = perm
            };
            ret.xAttributeType = eBTDbDescriptor;   
            ret.xCharacteristicDescr = xCharacteristicDescr;
            break;
        }
            // TODO
        case ESP_GATT_DB_INCLUDED_SERVICE:{
            BTIncludedService_t xIncludedService = {
                .xUuid = uuid,
                .pxPtrToService = NULL
            };
            ret.xAttributeType = eBTDbIncludedService;
            ret.xIncludedService = xIncludedService;
            break;
        }
        default:
            // ret = NULL;
            break;
    }
    return ret;
}

/** Get gatt db content */
BTStatus_t prvBTGetGattDb ( uint16_t usConnId, uint16_t* _count )
{
    BTStatus_t xStatus = eBTStatusSuccess;

    uint16_t count;
    xStatus = esp_ble_gattc_get_attr_count(ulGattClientIFhandle, usConnId, ESP_GATT_DB_ALL, 0, 0xffff, 0, &count);
    *_count = count;
    if(xStatus == ESP_OK)
    {
        esp_gattc_db_elem_t *db;
        db = (esp_gattc_db_elem_t *)calloc(count, sizeof(esp_gattc_db_elem_t));
        xStatus = esp_ble_gattc_get_db( ulGattClientIFhandle, usConnId, 0, 0xffff, db, &count);
        if(xStatus == ESP_OK)
        {
            if(xGattDB != NULL)
            {
                free(xGattDB);
            }
            // allocate memory for connId db elements
            xGattDB = (BTGattDbElement_t*)calloc(count, sizeof(BTGattDbElement_t));
            ESP_LOGI(__func__, "attr count: %d", count);

            for(int i=0; i<count; i++)
            {
                esp_gattc_db_elem_t el = db[i];
                BTAttribute_t attr = espDBElToDBAttribute(el);
                BTGattDbElement_t dbEl = {
                    .attribute_handle = el.attribute_handle,
                    .attribute = attr,
                    // .start_handle,
                    // .end_handle

                };
                
                memcpy(&xGattDB[i], &dbEl, sizeof(BTGattDbElement_t));

                // ESP_LOGI(__func__, "handle: %d, handle: %d, properties: %d, ", el.attribute_handle, xGattDB[i].attribute_handle, el.properties);
                // ESP_LOG_BUFFER_HEX(__func__, &xGattDB[i].attribute, sizeof(BTAttribute_t));
            }
        }
    free(db);
    }

    return xStatus;
}


const void * prvBTGetGattClientInterface()
{
    return &xGATTclientInterface;
}
