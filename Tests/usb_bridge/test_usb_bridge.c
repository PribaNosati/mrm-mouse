#include "app_usb_bridge.h"
#include "can_bus.h"
#include "usb_commands.h"
#include "usbd_cdc_if.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static USBD_CDC_HandleTypeDef cdc;
USBD_HandleTypeDef hUsbDeviceFS={&cdc,USBD_STATE_CONFIGURED};
static usb_command_send_fn sender;
static uint8_t *pending;
static unsigned sends,delays;
static int release;
void CanBus_ProcessRx(can_rx_send_fn send){(void)send;}
void MotorControl_Init(void){}
void MotorControl_Process(void){}
void UsbCommands_Poll(void){}
void UsbCommands_Init(usb_command_send_fn fn){sender=fn;}
void UsbCommands_Process(void){}
void UsbCommands_Receive(const uint8_t *data,uint32_t length){(void)data;(void)length;}
void HAL_Delay(uint32_t ms){(void)ms;delays++;if(release)cdc.TxState=0;}
uint8_t CDC_Transmit_FS(uint8_t *data,uint16_t length){(void)length;pending=data;cdc.TxState=1;sends++;return USBD_OK;}
int main(void){
 char reply[]="OK FIRST";
 AppUsbBridge_Init();sender(reply,8);memset(reply,'X',8);
 assert(memcmp(pending,"OK FIRST",8)==0 && sends==1);
 sender("SECOND",6);assert(sends==1 && delays==20 && memcmp(pending,"OK FIRST",8)==0);
 release=1;sender("SECOND",6);assert(sends==2 && memcmp(pending,"SECOND",6)==0);
 hUsbDeviceFS.pClassData=NULL;sender("THIRD",5);assert(sends==2);
 hUsbDeviceFS.pClassData=&cdc;hUsbDeviceFS.dev_state=0;sender("FOURTH",6);assert(sends==2);
 puts("PASS: USB reply owns buffer, busy transfer preserved, bounded wait, disconnected/null USB class");return 0;
}
