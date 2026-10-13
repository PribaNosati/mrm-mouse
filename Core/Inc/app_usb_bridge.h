#ifndef APP_USB_BRIDGE_H
#define APP_USB_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AppUsbBridge_Init(void);
void AppUsbBridge_Process(void);
void AppUsbBridge_Receive(const uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif

