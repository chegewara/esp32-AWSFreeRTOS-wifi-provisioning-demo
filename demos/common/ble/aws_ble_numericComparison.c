#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "iot_ble.h"
#include "iot_ble_config.h"
#include "aws_ble_numericComparison.h"

typedef struct{
	uint32_t ulPassKey;
	BTBdaddr_t xAdress;
} BLEPassKeyConfirm_t;

QueueHandle_t xNumericComparisonQueue = NULL;

void BLEGAPPairingStateChangedCb( BTStatus_t xStatus,
                                     BTBdaddr_t * pxRemoteBdAddr,
                                     BTSecurityLevel_t xSecurityLevel,
                                     BTAuthFailureReason_t xReason )
{
}

extern void _BLENumericComparisonCb(BTBdaddr_t * pxRemoteBdAddr, uint32_t ulPassKey);
void BLENumericComparisonCb(BTBdaddr_t * pxRemoteBdAddr, uint32_t ulPassKey)
{
	BLEPassKeyConfirm_t xPassKeyConfirm;

	if(pxRemoteBdAddr != NULL)
	{


		xPassKeyConfirm.ulPassKey = ulPassKey;
		memcpy(&xPassKeyConfirm.xAdress, pxRemoteBdAddr, sizeof(BTBdaddr_t));

		xQueueSend(xNumericComparisonQueue, (void * )&xPassKeyConfirm, (portTickType)portMAX_DELAY);
        _BLENumericComparisonCb(pxRemoteBdAddr, ulPassKey);
	}
}

void userInputTask(void *pvParameters)
{
	INPUTMessage_t xINPUTmessage;
    BLEPassKeyConfirm_t xPassKeyConfirm;
    TickType_t xAuthTimeout = pdMS_TO_TICKS( IOT_BLE_NUMERIC_COMPARISON_TIMEOUT_SEC * 1000 );

    for (;;) {
    	if (xQueueReceive(xNumericComparisonQueue, (void * )&xPassKeyConfirm, portMAX_DELAY ))
    	{
              configPRINTF(("Numeric comparison:%ld\n", xPassKeyConfirm.ulPassKey ));
              configPRINTF(("Press 'y' to confirm\n"));
              /* Waiting for UART event. */
              if ( getUserMessage( &xINPUTmessage, xAuthTimeout ) == pdTRUE ) {
                    if((xINPUTmessage.pcData[0] == 'y')||(xINPUTmessage.pcData[0] == 'Y'))
                    {
                    	configPRINTF(("Key accepted\n"));
                        BTStatus_t _status = IotBle_ConfirmNumericComparisonKeys(&xPassKeyConfirm.xAdress, true);
                        char status[] = "status: %d\n";
                        sprintf(status, status, _status);
                        configPRINTF((status));
                    }else
                    {
                    	configPRINTF(("Key Rejected\n"));
                        IotBle_ConfirmNumericComparisonKeys(&xPassKeyConfirm.xAdress, false);

                    }

                    vPortFree(xINPUTmessage.pcData);
              }
    	}
    }
    vTaskDelete(NULL);
}

void NumericComparisonInit(void)
{


	#if( IOT_BLE_ENABLE_NUMERIC_COMPARISON == 1 )
    /* Create a queue that will pass in the code to the UART task and wait validation from the user. */
    xNumericComparisonQueue = xQueueCreate( 1, sizeof( BLEPassKeyConfirm_t ) );
    xTaskCreate(userInputTask, "InputTask", 2048, NULL, 1, NULL);
    #endif

}
