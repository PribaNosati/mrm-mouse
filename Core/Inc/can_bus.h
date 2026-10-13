#ifndef CAN_BUS_H
#define CAN_BUS_H
#include "stm32g0xx_hal.h"
#include <stdbool.h>
#include <stddef.h>

/* Start interrupt reception after MX_FDCAN1_Init; report all STD/EXT IDs by default. */
HAL_StatusTypeDef CanBus_Init(void);

/* Call from the application task, not an interrupt. HAL_OK means queued. */
HAL_StatusTypeDef CanBus_Send(uint32_t identifier, bool extended,
                             const uint8_t *data, uint8_t length);
/* CAN STD|EXT <hex identifier> [0..8 hex bytes], or CAN STATUS. */
void CanBus_Command(const char *arguments, char *response, size_t capacity);
typedef bool (*can_rx_send_fn)(const char *text, uint16_t length);
/* Forward interrupt-captured frames from the application task; never blocks on USB. */
void CanBus_ProcessRx(can_rx_send_fn send);
#endif

