#include <stdint.h>
#include <stdbool.h>
#ifndef CAN_DEVICES_H
#define CAN_DEVICES_H

typedef enum
{
    LID,
    MOT,
    REF,
    COL,
    ULT
} can_device_type;

typedef union
{
    uint16_t lid_distance;
    int16_t mot_speed;
    uint16_t ref_distances[9];
    uint16_t col_colors[14];
    uint16_t ult_distance;
} can_device_value;

typedef struct {
    uint32_t id_in; // id_out is not needed, because it is always id_in + 1
    bool extended; // true for extended 29-bit identifier, false for standard 11-bit identifier
    can_device_type type; // LID, MOT, REF, COL, or ULT
    volatile can_device_value value; // Measurements, etc. from the device, updated when a message is received
} can_device;

enum { CAN_DEVICE_COUNT = 16U };

extern can_device can_devices[CAN_DEVICE_COUNT];
extern volatile uint32_t can_devices_count;

void CanDevices_ResetActive(void);
void CanDevices_HandleRx(uint32_t identifier, bool extended,
                         const uint8_t *payload, uint8_t payload_length);
#endif
