#include "usbpd_sink_policy.h"
#include "usbpd_vbus.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define FIXED(mv, ma) ((((mv) / 50U) << 10) | ((ma) / 10U))

static void check_request(const UsbPdSinkRequest *request, uint32_t position,
                          uint32_t mv, uint32_t ma, bool target)
{
    assert(request->object_position == position);
    assert(request->voltage_mv == mv);
    assert(request->current_ma == ma);
    assert(request->target_available == target);
    assert(((request->rdo >> 28) & 7U) == position);
    assert(((request->rdo >> 10) & 0x3FFU) == ma / 10U);
    assert((request->rdo & 0x3FFU) == ma / 10U);
    assert(((request->rdo >> 26) & 1U) == (target ? 0U : 1U));
    assert((request->rdo & (1UL << 25)) != 0U);
    assert((request->rdo & (1UL << 24)) != 0U);
    assert((request->rdo & (1UL << 27)) == 0U); /* No GiveBack. */
}

int main(void)
{
    UsbPdSinkRequest request;
    uint32_t pdos[7] = {FIXED(5000, 3000), FIXED(9000, 3000),
                        FIXED(15000, 3000), FIXED(20000, 5000)};
    assert(UsbPdSink_SelectRequest(pdos, 4U, &request));
    check_request(&request, 2U, 9000U, 3000U, true);

    /* Use the source's actual object position, including the final slot. */
    pdos[1] = FIXED(12000, 3000);
    pdos[6] = FIXED(9000, 5000);
    assert(UsbPdSink_SelectRequest(pdos, 7U, &request));
    check_request(&request, 7U, 9000U, 3000U, true);

    /* 9 V / 2 A is insufficient; PPS/variable/battery entries must not be
       mistaken for a fixed 9 V offer. Higher voltages are never selected. */
    for (uint32_t type = 1U; type <= 3U; ++type)
    {
        pdos[1] = FIXED(9000, 2000);
        pdos[6] = (type << 30) | FIXED(9000, 3000);
        assert(UsbPdSink_SelectRequest(pdos, 7U, &request));
        check_request(&request, 1U, 5000U, 500U, false);
    }

    pdos[0] = FIXED(5000, 100);
    assert(UsbPdSink_SelectRequest(pdos, 1U, &request));
    check_request(&request, 1U, 5000U, 100U, false);
    pdos[0] = FIXED(5000, 0);
    assert(!UsbPdSink_SelectRequest(pdos, 1U, &request));
    assert(request.rdo == 0U);
    pdos[0] = FIXED(20000, 3000);
    assert(!UsbPdSink_SelectRequest(pdos, 1U, &request));
    assert(!UsbPdSink_SelectRequest(NULL, 1U, &request));
    assert(!UsbPdSink_SelectRequest(pdos, 0U, &request));
    assert(!UsbPdSink_SelectRequest(pdos, 8U, &request));
    assert(!UsbPdSink_SelectRequest(pdos, 1U, NULL));
    assert(request.rdo == 0U && !request.target_available);

    assert(((USBPD_SINK_PDO_5V >> 10) & 0x3FFU) == 100U);
    assert((USBPD_SINK_PDO_5V & 0x3FFU) == 50U);
    assert(((USBPD_SINK_PDO_9V >> 10) & 0x3FFU) == 180U);
    assert((USBPD_SINK_PDO_9V & 0x3FFU) == 300U);
    /* Nominal readings from the actual 200 kOhm / 10.2 kOhm divider.
       A 12-bit ADC step corresponds to about 16.6 mV of VBUS at 3.3 V. */
    assert(UsbPdVbus_FromAdc(0U, 3300U) == 0U);
    uint32_t measured = UsbPdVbus_FromAdc(542U, 3300U);
    assert(measured >= 8990U && measured <= 9010U);
    measured = UsbPdVbus_FromAdc(301U, 3300U);
    assert(measured >= 4990U && measured <= 5010U);
    measured = UsbPdVbus_FromAdc(596U, 3000U);
    assert(measured >= 8990U && measured <= 9010U);
    puts("PASS: USB PD selects 9 V / 3 A, bounds current and falls back to 5 V");
    return 0;
}
