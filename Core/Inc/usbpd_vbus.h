#ifndef USBPD_VBUS_H
#define USBPD_VBUS_H

#include <stdint.h>

/* Mouse: VBUS -- 200 kOhm -- PA5/ADC_VBUS -- 10.2 kOhm -- GND. */
#define USBPD_VBUS_TOP_OHMS       200000U
#define USBPD_VBUS_BOTTOM_OHMS     10200U

static inline uint32_t UsbPdVbus_FromAdc(uint32_t raw, uint32_t vdda_mv)
{
    const uint32_t denominator = 4095U * USBPD_VBUS_BOTTOM_OHMS;
    uint64_t numerator = (uint64_t)raw * vdda_mv *
                        (USBPD_VBUS_TOP_OHMS + USBPD_VBUS_BOTTOM_OHMS);
    return (uint32_t)((numerator + denominator / 2U) / denominator);
}

#endif
