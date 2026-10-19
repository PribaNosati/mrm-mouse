#include "motor_control.h"
#include "main.h"

extern TIM_HandleTypeDef htim2;

static const uint32_t channels[MOTOR_COUNT] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};
static GPIO_TypeDef *const direction_ports[MOTOR_COUNT] = {
    MOTOR1_DIR_GPIO_Port, MOTOR2_DIR_GPIO_Port,
    MOTOR3_DIR_GPIO_Port, MOTOR4_DIR_GPIO_Port
};
static const uint16_t direction_pins[MOTOR_COUNT] = {
    MOTOR1_DIR_Pin, MOTOR2_DIR_Pin, MOTOR3_DIR_Pin, MOTOR4_DIR_Pin
};
static int16_t speeds[MOTOR_COUNT];
static bool enabled;
static bool testing;
static uint32_t test_tick;
static int16_t test_speed;
static int16_t test_step;

void MotorControl_StopAll(void)
{
    testing = false;
    for (uint8_t i = 0U; i < MOTOR_COUNT; ++i)
    {
        __HAL_TIM_SET_COMPARE(&htim2, channels[i], 0U);
        speeds[i] = 0;
    }
}

void MotorControl_Enable(bool enable)
{
    MotorControl_StopAll();
    enabled = enable;
    HAL_GPIO_WritePin(NSLEEP_MOTORS_GPIO_Port, NSLEEP_MOTORS_Pin,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void MotorControl_Init(void)
{
    MotorControl_Enable(true);
    for (uint8_t i = 0U; i < MOTOR_COUNT; ++i)
    {
        HAL_GPIO_WritePin(direction_ports[i], direction_pins[i], GPIO_PIN_RESET);
        if (HAL_TIM_PWM_Start(&htim2, channels[i]) != HAL_OK)
        {
            MotorControl_EmergencyStop();
            Error_Handler();
        }
    }
}

bool MotorControl_IsEnabled(void) { return enabled; }
void MotorControl_CancelTest(void) { testing = false; }

bool MotorControl_Set(uint8_t motor_number, int16_t signed_percent)
{
    if ((motor_number < 1U) || (motor_number > MOTOR_COUNT) ||
        (signed_percent < -100) || (signed_percent > 100) || !enabled)
        return false;

    uint8_t index = motor_number - 1U;
    uint32_t magnitude = (uint32_t)(signed_percent < 0 ?
                                   -signed_percent : signed_percent);
    /* Remove drive before switching direction. Preserve CubeMX PWM polarity. */
    __HAL_TIM_SET_COMPARE(&htim2, channels[index], 0U);
    HAL_GPIO_WritePin(direction_ports[index], direction_pins[index],
                      signed_percent > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim2, channels[index],
                         ((__HAL_TIM_GET_AUTORELOAD(&htim2) + 1U) * magnitude) / 100U);
    speeds[index] = signed_percent;
    return true;
}

int16_t MotorControl_Get(uint8_t motor_number)
{
    return ((motor_number >= 1U) && (motor_number <= MOTOR_COUNT)) ?
           speeds[motor_number - 1U] : 0;
}

void MotorControl_EmergencyStop(void) { MotorControl_Enable(false); }

bool MotorControl_StartTest(void)
{
    if (!enabled) return false;
    MotorControl_StopAll();
    test_speed = 0;
    test_step = 1;
    test_tick = HAL_GetTick();
    testing = true;
    return true;
}

void MotorControl_Process(void)
{
    uint32_t now = HAL_GetTick();
    if (!testing || ((uint32_t)(now - test_tick) < 50U)) return;
    test_tick = now;
    test_speed += test_step;
    for (uint8_t i = 1U; i <= MOTOR_COUNT; ++i)
        MotorControl_Set(i, test_speed);
    if (test_speed == 100) test_step = -1;
    else if (test_speed == -100) test_step = 1;
    else if ((test_speed == 0) && (test_step == 1)) MotorControl_StopAll();
}
