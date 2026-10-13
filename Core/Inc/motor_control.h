#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOTOR_COUNT 4U

void MotorControl_Init(void);
void MotorControl_Enable(bool enable);
bool MotorControl_IsEnabled(void);
bool MotorControl_Set(uint8_t motor_number, int16_t signed_percent);
int16_t MotorControl_Get(uint8_t motor_number);
void MotorControl_StopAll(void);
void MotorControl_EmergencyStop(void);
bool MotorControl_StartTest(void);
void MotorControl_CancelTest(void);
void MotorControl_Process(void);

#ifdef __cplusplus
}
#endif

#endif

