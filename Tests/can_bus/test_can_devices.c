#include "can_devices.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    const uint8_t lid_alive[] = {0xFFU, (uint8_t)LID, 0U};
    const uint8_t legacy_lid_alive[] = {0xFFU};
    const uint8_t mot_alive[] = {0xFFU, (uint8_t)MOT, 0U};
    const uint8_t col_alive[] = {0xFFU, (uint8_t)COL, 0U};
    const uint8_t ult_alive[] = {0xFFU, (uint8_t)ULT, 0U};
    const uint8_t bad_alive[] = {0xFFU, 0xFFU, 0U};
    const uint8_t lid_distance[] = {0x13U, 0x34U, 0x12U};
    const uint8_t another_distance[] = {0x13U, 0x78U, 0x56U};
    const uint8_t ult_distance[] = {
        COMMAND_SENSORS_MEASURE_SENDING, 0xCDU, 0xABU};
    const uint8_t short_ult_distance[] = {
        COMMAND_SENSORS_MEASURE_SENDING, 0xEFU};

    CanDevices_ResetActive();
    assert(can_devices_count == 0U);

    CanDevices_HandleRx(0U, false, lid_alive, sizeof(lid_alive));
    CanDevices_HandleRx(0x181U, false, NULL, 0U);
    CanDevices_HandleRx(0x181U, false, bad_alive, sizeof(bad_alive));
    assert(can_devices_count == 0U);

    CanDevices_HandleRx(0x181U, false, legacy_lid_alive,
                        sizeof(legacy_lid_alive));
    assert(can_devices_count == 1U);
    assert(can_devices[0].id_in == 0x180U);
    assert(!can_devices[0].extended);
    assert(can_devices[0].type == LID);
    assert(can_devices[0].value.lid_distance == 0U);

    /* A 0x13 payload updates an existing LID without creating a duplicate. */
    CanDevices_HandleRx(0x181U, false, lid_distance, sizeof(lid_distance));
    assert(can_devices_count == 1U);
    assert(can_devices[0].type == LID);
    assert(can_devices[0].value.lid_distance == 0x1234U);

    /* A repeated alive report does not clear the latest measurement. */
    CanDevices_HandleRx(0x181U, false, lid_alive, sizeof(lid_alive));
    assert(can_devices_count == 1U);
    assert(can_devices[0].value.lid_distance == 0x1234U);

    /* The same numeric ID in the other CAN format is a distinct device. */
    CanDevices_HandleRx(0x181U, true, mot_alive, sizeof(mot_alive));
    assert(can_devices_count == 2U);
    assert(can_devices[1].id_in == 0x180U);
    assert(can_devices[1].extended);
    assert(can_devices[1].type == MOT);
    assert(can_devices[1].value.mot_speed == 0);
    CanDevices_HandleRx(0x181U, true, another_distance,
                        sizeof(another_distance));
    assert(can_devices[1].value.mot_speed == 0);

    CanDevices_HandleRx(0x183U, false, col_alive, sizeof(col_alive));
    assert(can_devices_count == 3U);
    assert(can_devices[2].type == COL);
    const uint8_t colors_1_to_3[] = {COMMAND_SENDING_COLORS_1_TO_3,
        0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U};
    const uint8_t colors_4_to_6[] = {COMMAND_SENDING_COLORS_4_TO_6,
        0x07U, 0x08U, 0x09U, 0x0AU, 0x0BU, 0x0CU};
    const uint8_t colors_7_to_9[] = {COMMAND_SENDING_COLORS_7_TO_9,
        0x0DU, 0x0EU, 0x0FU, 0x10U, 0x11U, 0x12U};
    const uint8_t colors_10_to_12[] = {COMMAND_SENDING_COLORS_10_TO_12,
        0x13U, 0x14U, 0x15U, 0x16U, 0x17U, 0x18U};
    const uint8_t colors_13_to_14[] = {COMMAND_SENDING_COLORS_13_TO_14,
        0x19U, 0x1AU, 0x1BU, 0x1CU};
    CanDevices_HandleRx(0x183U, false, colors_1_to_3,
                        sizeof(colors_1_to_3));
    CanDevices_HandleRx(0x183U, false, colors_4_to_6,
                        sizeof(colors_4_to_6));
    CanDevices_HandleRx(0x183U, false, colors_7_to_9,
                        sizeof(colors_7_to_9));
    CanDevices_HandleRx(0x183U, false, colors_10_to_12,
                        sizeof(colors_10_to_12));
    CanDevices_HandleRx(0x183U, false, colors_13_to_14,
                        sizeof(colors_13_to_14));
    for (uint32_t color = 0U; color < 14U; ++color)
    {
        uint16_t expected = (uint16_t)(((2U * color) + 1U) << 8) |
                            (uint16_t)((2U * color) + 2U);
        assert(can_devices[2].value.col_colors[color] == expected);
    }

    /* ULT measurements are unsigned 16-bit values sent low byte first. */
    CanDevices_HandleRx(0x401U, false, ult_alive, sizeof(ult_alive));
    assert(can_devices_count == 4U);
    assert(can_devices[3].type == ULT);
    assert(can_devices[3].value.ult_distance == 0U);
    CanDevices_HandleRx(0x401U, false, ult_distance, sizeof(ult_distance));
    assert(can_devices[3].value.ult_distance == 0xABCDU);
    CanDevices_HandleRx(0x401U, false, short_ult_distance,
                        sizeof(short_ult_distance));
    assert(can_devices[3].value.ult_distance == 0xABCDU);

    /* All remaining entries, including the final array slot, are usable. */
    for (uint32_t i = 4U; i < CAN_DEVICE_COUNT; ++i)
        CanDevices_HandleRx(0x181U + i, false, mot_alive, sizeof(mot_alive));
    assert(can_devices_count == CAN_DEVICE_COUNT);
    assert(can_devices[CAN_DEVICE_COUNT - 1U].id_in == 0x18FU);

    CanDevices_HandleRx(0x400U, false, lid_alive, sizeof(lid_alive));
    assert(can_devices_count == CAN_DEVICE_COUNT);

    CanDevices_ResetActive();
    assert(can_devices_count == 0U);

    puts("PASS: CAN devices update LID and ULT values, reject duplicates, and use all slots");
    return 0;
}
