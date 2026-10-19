#ifndef USBPD_SINK_POLICY_H
#define USBPD_SINK_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#define USBPD_SINK_TARGET_MV       9000U
#define USBPD_SINK_TARGET_MA       3000U
#define USBPD_SINK_FALLBACK_MV     5000U
#define USBPD_SINK_FALLBACK_MA      500U
#define USBPD_SINK_MAX_SOURCE_PDOS    7U

/* Fixed sink PDOs: voltage in 50 mV units, current in 10 mA units.
   Only the first PDO carries the higher-capability and USB-data flags. */
#define USBPD_SINK_PDO_5V ((1UL << 28) | (1UL << 26) | \
                         ((USBPD_SINK_FALLBACK_MV / 50U) << 10) | \
                         (USBPD_SINK_FALLBACK_MA / 10U))
#define USBPD_SINK_PDO_9V (((USBPD_SINK_TARGET_MV / 50U) << 10) | \
                         (USBPD_SINK_TARGET_MA / 10U))

typedef struct
{
    uint32_t rdo;
    uint32_t voltage_mv;
    uint32_t current_ma;
    uint8_t object_position;
    bool target_available;
} UsbPdSinkRequest;

/* Prefer exactly 9 V with at least 3 A; otherwise use 5 V at up to 500 mA.
   Invalid input or no usable fixed profile returns false and a zero request. */
bool UsbPdSink_SelectRequest(const uint32_t *source_pdos, uint32_t count,
                            UsbPdSinkRequest *request);

#endif
