/**
Purpose: common library, the same for all MRMS ARM firmware
@author MRMS team
@version 0.1 2019-09-08
Licence: You can use this code any way you like.
*/

#ifndef COMMON_H_
#define COMMON_H_

//#include <stdlib.h>
#include <stdint.h>


// Definitions

#define COMMAND_SENDING_COLORS_1_TO_3 0x06
#define COMMAND_SENDING_COLORS_4_TO_6 0x07
#define COMMAND_SENDING_COLORS_7_TO_9 0x08
#define COMMAND_SENDING_COLORS_10_TO_12 0x09
#define COMMAND_SENDING_COLORS_13_TO_14 0x0A
#define COMMAND_SENSORS_MEASURE_CONTINUOUS 0x10
#define COMMAND_SENSORS_MEASURE_ONCE 0x11
#define COMMAND_SENSORS_MEASURE_STOP 0x12
#define COMMAND_SENSORS_MEASURE_SENDING 0x13
#define COMMAND_SENSORS_MEASURE_CONTINUOUS_REQUEST_NOTIFICATION 0x14
#define COMMAND_SENSORS_MEASURE_CONTINUOUS_AND_RETURN_CALCULATED_DATA 0x15
#define COMMAND_SENSORS_MEASURE_CALCULATED_SENDING 0x16
#define COMMAND_SENSORS_MEASURE_CONTINUOUS_VERSION_2 0x17
#define COMMAND_SENSORS_MEASURE_CONTINUOUS_VERSION_3 0x18
#define COMMAND_FIRMWARE_REQUEST 0x19
#define COMMAND_FIRMWARE_SENDING 0x1A
#define COMMAND_RESET 0x1B
#define COMMAND_MESSAGE_SENDING_1 0x1C
#define COMMAND_MESSAGE_SENDING_2 0x1D
#define COMMAND_MESSAGE_SENDING_3 0x1E
#define COMMAND_MESSAGE_SENDING_4 0x1F
#define COMMAND_SPEED_SET 0x20
#define COMMAND_SPEED_SET_REQUEST_NOTIFICATION 0x21
#define COMMAND_DUPLICATE_ID_PING 0x22
#define COMMAND_DUPLICATE_ID_ECHO 0x23
#define COMMAND_INFO_REQUEST 0x24
#define COMMAND_INFO_SENDING_1 0x25
#define COMMAND_INFO_SENDING_2 0x26
#define COMMAND_INFO_SENDING_3 0x27
#define COMMAND_FPS_REQUEST 0x30
#define COMMAND_FPS_SENDING 0x31
#define COMMAND_ID_CHANGE_REQUEST 0x40
#define COMMAND_NOTIFICATION 0x41
#define COMMAND_OSCILLATOR_TEST 0x43
// 0x50 - 0x8F reserved for non-common messages
#define COMMAND_COL_ILLUMINATION 0x50
#define COMMAND_CAN_TEST 0xFE
#define COMMAND_ERROR 0xEE
#define COMMAND_REPORT_ALIVE 0xFF

#ifndef PI
#define PI 3.14159265359
#endif

#define STM32_UUID ((uint32_t *)0x1FFF7590) // STM32G0B1 unique id

#ifndef toRad
#define toRad(x) ((x) / 180.0 * PI) // Degrees to radians
#endif
#ifndef toDeg
#define toDeg(x) ((x) / PI * 180.0) // Radians to degrees
#endif

#define WARNING_LIMIT 100 // Below limit warnings, above error


// Functions

float constrain(float value, float minimum, float maximum);

uint8_t error();


float map(float x, float in_min, float in_max, float out_min, float out_max);

#endif /* COMMON_H_ */
