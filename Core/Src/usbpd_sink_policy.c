#include "usbpd_sink_policy.h"
#include <stddef.h>

bool UsbPdSink_SelectRequest(const uint32_t *source_pdos, uint32_t count,
                            UsbPdSinkRequest *request)
{
    uint32_t fallback = USBPD_SINK_MAX_SOURCE_PDOS;
    uint32_t selected = USBPD_SINK_MAX_SOURCE_PDOS;
    uint32_t current_units;

    if (request == NULL) return false;
    *request = (UsbPdSinkRequest){0};
    if ((source_pdos == NULL) || (count == 0U) ||
        (count > USBPD_SINK_MAX_SOURCE_PDOS)) return false;

    for (uint32_t i = 0U; i < count; ++i)
    {
        uint32_t pdo = source_pdos[i];
        uint32_t voltage = ((pdo >> 10) & 0x3FFU) * 50U;
        uint32_t current = (pdo & 0x3FFU) * 10U;
        if (((pdo >> 30) != 0U) || (current == 0U)) continue;
        if ((voltage == USBPD_SINK_FALLBACK_MV) &&
            (fallback == USBPD_SINK_MAX_SOURCE_PDOS)) fallback = i;
        if ((voltage == USBPD_SINK_TARGET_MV) &&
            (current >= USBPD_SINK_TARGET_MA))
        {
            selected = i;
            break;
        }
    }

    request->target_available = selected != USBPD_SINK_MAX_SOURCE_PDOS;
    if (!request->target_available) selected = fallback;
    if (selected == USBPD_SINK_MAX_SOURCE_PDOS) return false;

    request->object_position = (uint8_t)(selected + 1U);
    request->voltage_mv = request->target_available ? USBPD_SINK_TARGET_MV :
                                                      USBPD_SINK_FALLBACK_MV;
    current_units = request->target_available ? USBPD_SINK_TARGET_MA / 10U :
                                               USBPD_SINK_FALLBACK_MA / 10U;
    if (current_units > (source_pdos[selected] & 0x3FFU))
        current_units = source_pdos[selected] & 0x3FFU;
    request->current_ma = current_units * 10U;
    /* Fixed RDO: position, USB communications, no USB suspend, and the
       operating/maximum currents. Fallback reports capability mismatch. */
    request->rdo = ((uint32_t)request->object_position << 28) |
                   (1UL << 25) | (1UL << 24) |
                   (request->target_available ? 0U : (1UL << 26)) |
                   (current_units << 10) | current_units;
    return true;
}
