#include "can_devices.h"
#include "common.h"
#include <stddef.h>

can_device can_devices[CAN_DEVICE_COUNT] = {0};
volatile uint32_t can_devices_count;

static void update_col_values(can_device *device, const uint8_t *payload,
                              uint8_t payload_length)
{
    uint32_t first;
    uint32_t count;

    switch (payload[0])
    {
        case COMMAND_SENDING_COLORS_1_TO_3:
            first = 0U;
            count = 3U;
            break;
        case COMMAND_SENDING_COLORS_4_TO_6:
            first = 3U;
            count = 3U;
            break;
        case COMMAND_SENDING_COLORS_7_TO_9:
            first = 6U;
            count = 3U;
            break;
        case COMMAND_SENDING_COLORS_10_TO_12:
            first = 9U;
            count = 3U;
            break;
        case COMMAND_SENDING_COLORS_13_TO_14:
            first = 12U;
            count = 2U;
            break;
        default:
            return;
    }

    if (payload_length < (uint8_t)(1U + (2U * count)))
        return;

    for (uint32_t color = 0U; color < count; ++color)
    {
        uint32_t byte = 1U + (2U * color);
        device->value.col_colors[first + color] =
            ((uint16_t)payload[byte] << 8) | (uint16_t)payload[byte + 1U];
    }
}

void CanDevices_ResetActive(void)
{
    can_devices_count = 0U;
}

void CanDevices_HandleRx(uint32_t identifier, bool extended,
                         const uint8_t *payload, uint8_t payload_length)
{
    uint32_t i;
    if ((identifier == 0U) || (payload == NULL) || (payload_length == 0U))
        return;

    for (i = 0U; i < can_devices_count; ++i)
    {
        if ((can_devices[i].extended == extended) &&
            (identifier == (can_devices[i].id_in + 1U)))
        {
            if ((payload_length >= 3U) &&
                (can_devices[i].type == LID) &&
                (payload[0] == COMMAND_SENSORS_MEASURE_SENDING))
            {
                can_devices[i].value.lid_distance =
                    (uint16_t)payload[1] | ((uint16_t)payload[2] << 8);
            }
            else if ((payload_length >= 3U) &&
                     (can_devices[i].type == ULT) &&
                     (payload[0] == COMMAND_SENSORS_MEASURE_SENDING))
            {
                can_devices[i].value.ult_distance =
                    (uint16_t)payload[1] | ((uint16_t)payload[2] << 8);
            }
            else if (can_devices[i].type == COL)
            {
                update_col_values(&can_devices[i], payload, payload_length);
            }
            return;
        }
    }

    if ((payload[0] == COMMAND_REPORT_ALIVE) &&
        (can_devices_count < CAN_DEVICE_COUNT))
    {
        uint8_t device_type = (payload_length >= 2U) ?
                              payload[1] : (uint8_t)LID;
        if (device_type > (uint8_t)ULT)
            return;

        uint32_t new_device = can_devices_count;
        can_devices[new_device].id_in = identifier - 1U;
        can_devices[new_device].extended = extended;
        can_devices[new_device].type = (can_device_type)device_type;
        can_devices[new_device].value = (can_device_value){0};
        can_devices_count = new_device + 1U;
    }
}
