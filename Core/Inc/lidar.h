#ifndef LIDAR_H
#define LIDAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Start or stop measurements for a validated device number (1..16). */
void lidarStart(uint8_t device_nr, bool start, char *response, size_t capacity);
bool lidarIsStarted(uint8_t device_nr);

#ifdef __cplusplus
}
#endif

#endif
