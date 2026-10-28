#include "lidar.h"
#include "can_bus.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

static bool started[16];

bool lidarIsStarted(uint8_t device_nr)
{
    return (device_nr >= 1U) && (device_nr <= 16U) && started[device_nr - 1U];
}

void lidarStart(uint8_t device_nr, bool start, char *response, size_t capacity)
{
    char arguments[32];
    char result[160];
    if ((device_nr < 1U) || (device_nr > 16U))
    {
        snprintf(response, capacity, "ERR device number=1..16\r\n");
        return;
    }
    uint32_t identifier = 0x180U + (2U * ((uint32_t)device_nr - 1U));

    snprintf(arguments, sizeof(arguments), "std 0x%lx %02x",
             (unsigned long)identifier,
             start ? COMMAND_SENSORS_MEASURE_CONTINUOUS :
                     COMMAND_SENSORS_MEASURE_STOP);
    CanBus_Command(arguments, result, sizeof(result));
    if (strncmp(result, "OK ", 3U) == 0)
        started[device_nr - 1U] = start;
    snprintf(response, capacity, "%s", result);
}
