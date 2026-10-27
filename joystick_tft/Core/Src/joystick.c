#include "joystick.h"
#include "adc.h"

#define ADC_LOW  1300U
#define ADC_HIGH 2800U

static uint16_t joystick_read_axis(uint32_t channel)
{
    ADC_ChannelConfTypeDef config = {0};

    config.Channel = channel;
    config.Rank = ADC_REGULAR_RANK_1;
    config.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    config.SingleDiff = ADC_SINGLE_ENDED;
    config.OffsetNumber = ADC_OFFSET_NONE;
    config.Offset = 0U;

    if (HAL_ADC_ConfigChannel(&hadc1, &config) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK)
    {
        Error_Handler();
    }

    return (uint16_t)HAL_ADC_GetValue(&hadc1);
}

JoystickState joystick_read(void)
{
    JoystickState state;
    uint8_t x_left;
    uint8_t x_right;
    uint8_t y_up;
    uint8_t y_down;

    state.x = joystick_read_axis(ADC_CHANNEL_5);
    state.y = joystick_read_axis(ADC_CHANNEL_6);
    state.pressed = HAL_GPIO_ReadPin(JOYSTICK_SW_GPIO_Port,
                                     JOYSTICK_SW_Pin) == GPIO_PIN_RESET;

    x_left = state.x < ADC_LOW;
    x_right = state.x > ADC_HIGH;
    y_up = state.y < ADC_LOW;
    y_down = state.y > ADC_HIGH;

    if (y_up && x_left) state.direction = JOY_UP_LEFT;
    else if (y_up && x_right) state.direction = JOY_UP_RIGHT;
    else if (y_down && x_left) state.direction = JOY_DOWN_LEFT;
    else if (y_down && x_right) state.direction = JOY_DOWN_RIGHT;
    else if (y_up) state.direction = JOY_UP;
    else if (y_down) state.direction = JOY_DOWN;
    else if (x_left) state.direction = JOY_LEFT;
    else if (x_right) state.direction = JOY_RIGHT;
    else state.direction = JOY_CENTER;

    return state;
}
