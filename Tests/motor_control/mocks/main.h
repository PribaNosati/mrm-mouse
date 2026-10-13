#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
typedef struct { uint32_t compare[4]; uint32_t period; } TIM_HandleTypeDef;
typedef struct { unsigned unused; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
extern GPIO_TypeDef mock_directions, mock_sleep;
#define HAL_OK 0
#define TIM_CHANNEL_1 0U
#define TIM_CHANNEL_2 4U
#define TIM_CHANNEL_3 8U
#define TIM_CHANNEL_4 12U
#define MOTOR1_DIR_GPIO_Port (&mock_directions)
#define MOTOR2_DIR_GPIO_Port (&mock_directions)
#define MOTOR3_DIR_GPIO_Port (&mock_directions)
#define MOTOR4_DIR_GPIO_Port (&mock_directions)
#define MOTOR1_DIR_Pin 1U
#define MOTOR2_DIR_Pin 2U
#define MOTOR3_DIR_Pin 4U
#define MOTOR4_DIR_Pin 8U
#define NSLEEP_MOTORS_GPIO_Port (&mock_sleep)
#define NSLEEP_MOTORS_Pin 16U
#define __HAL_TIM_SET_COMPARE(timer, channel, value) ((timer)->compare[(channel)/4U] = (value))
#define __HAL_TIM_GET_AUTORELOAD(timer) ((timer)->period)
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *timer, uint32_t channel);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
uint32_t HAL_GetTick(void);
void Error_Handler(void);
#endif
