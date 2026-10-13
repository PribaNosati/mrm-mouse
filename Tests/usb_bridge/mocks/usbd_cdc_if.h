#ifndef MOCK_USB_H
#define MOCK_USB_H
#include <stdint.h>
typedef struct {volatile uint32_t TxState;} USBD_CDC_HandleTypeDef;
typedef struct {void *pClassData;uint8_t dev_state;} USBD_HandleTypeDef;
#define USBD_STATE_CONFIGURED 3U
#define USBD_OK 0U
uint8_t CDC_Transmit_FS(uint8_t *data,uint16_t length);
#endif
