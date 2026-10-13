#include "motor_control.h"
#include "main.h"
#include <assert.h>
#include <stdio.h>

TIM_HandleTypeDef htim2 = {{0}, 99U};
GPIO_TypeDef mock_directions, mock_sleep;
static uint32_t tick;
static unsigned started, directions;
static GPIO_PinState sleep_state;
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *timer, uint32_t channel)
{
    assert(timer == &htim2 && channel / 4U < MOTOR_COUNT);
    started |= 1U << (channel / 4U);
    return HAL_OK;
}
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    if (port == &mock_sleep) sleep_state = state;
    else if (state == GPIO_PIN_SET) directions |= pin;
    else directions &= ~pin;
}
uint32_t HAL_GetTick(void) { return tick; }
void Error_Handler(void) { assert(0); }
int main(void)
{
    MotorControl_Init();
    assert(started == 15U && !MotorControl_IsEnabled() && sleep_state == GPIO_PIN_RESET);
    assert(!MotorControl_Set(1U, 50) && !MotorControl_StartTest());
    MotorControl_Enable(true);
    for (uint8_t i = 1U; i <= MOTOR_COUNT; ++i)
    {
        assert(MotorControl_Set(i, 100));
        assert(htim2.compare[i - 1U] == 100U && MotorControl_Get(i) == 100);
        assert(directions & (1U << (i - 1U)));
    }
    assert(!MotorControl_Set(0U, 10) && !MotorControl_Set(5U, 10));
    assert(!MotorControl_Set(1U, 101) && !MotorControl_Set(1U, -101));
    assert(MotorControl_Set(2U, -42) && htim2.compare[1] == 42U);
    assert(!(directions & MOTOR2_DIR_Pin) && MotorControl_Get(1U) == 100);
    MotorControl_Enable(false);
    for (uint8_t i = 0U; i < MOTOR_COUNT; ++i) assert(htim2.compare[i] == 0U);
    assert(sleep_state == GPIO_PIN_RESET && MotorControl_Get(2U) == 0);
    MotorControl_Enable(true);
    assert(MotorControl_StartTest());
    for (unsigned step = 0U; step < 400U; ++step)
    {
        tick += 50U;
        MotorControl_Process();
        int expected = step < 100U ? (int)step + 1 :
                       step < 300U ? 199 - (int)step : (int)step - 399;
        for (uint8_t i = 1U; i <= MOTOR_COUNT; ++i)
            assert(MotorControl_Get(i) == expected);
    }
    tick += 50U; MotorControl_Process();
    assert(MotorControl_Get(1U) == 0);
    assert(MotorControl_StartTest());
    tick += 50U; MotorControl_Process();
    MotorControl_StopAll();
    tick += 50U; MotorControl_Process();
    assert(MotorControl_Get(1U) == 0);
    MotorControl_EmergencyStop();
    assert(!MotorControl_IsEnabled() && sleep_state == GPIO_PIN_RESET);
    puts("PASS: four PWM channels, signed speed, bounds, disable/stop and interruptible finite motor test");
    return 0;
}
