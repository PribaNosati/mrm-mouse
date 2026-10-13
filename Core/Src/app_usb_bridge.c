#include "app_usb_bridge.h"
#include "motor_control.h"
#include "usb_commands.h"
#include "usbd_cdc_if.h"
#include "main.h"
#include "can_bus.h"
#include <string.h>

#define USB_TX_RETRY_ATTEMPTS 20U
extern USBD_HandleTypeDef hUsbDeviceFS;

static bool usb_try_send(const char *data, uint16_t length)
{
    static uint8_t tx_buffer[256];
    USBD_CDC_HandleTypeDef *cdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
    if ((data == NULL) || (length > sizeof(tx_buffer)) ||
        (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) || (cdc == NULL) ||
        (cdc->TxState != 0U)) return false;
    memcpy(tx_buffer, data, length);
    return CDC_Transmit_FS(tx_buffer, length) == USBD_OK;
}

static void usb_send(const char *data, uint16_t length)
{
    /* Allow the host several USB polling intervals to finish the previous packet. */
    for (uint32_t attempts = 0; attempts <= USB_TX_RETRY_ATTEMPTS; ++attempts)
    {
        if (usb_try_send(data, length)) return;
        if ((hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) ||
            (hUsbDeviceFS.pClassData == NULL) ||
            (attempts == USB_TX_RETRY_ATTEMPTS)) return;
        UsbCommands_Poll();
        HAL_Delay(1U);
    }
}

void AppUsbBridge_Init(void)
{
    MotorControl_Init();
    UsbCommands_Init(usb_send);
}

void AppUsbBridge_Process(void)
{
    MotorControl_Process();
    UsbCommands_Process();
    CanBus_ProcessRx(usb_try_send);
}
void AppUsbBridge_Receive(const uint8_t *data, uint32_t length)
{
    UsbCommands_Receive(data, length);
}

