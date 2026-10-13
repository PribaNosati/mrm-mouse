#ifndef USB_COMMANDS_H
#define USB_COMMANDS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*usb_command_send_fn)(const char *data, uint16_t length);

void UsbCommands_Init(usb_command_send_fn send_function);
/* Service background work (USB PD) during bounded command waits. */
void UsbCommands_SetPollFunction(void (*poll_function)(void));
void UsbCommands_Poll(void);
void UsbCommands_Receive(const uint8_t *data, uint32_t length);
void UsbCommands_Process(void);

#ifdef __cplusplus
}
#endif

#endif

