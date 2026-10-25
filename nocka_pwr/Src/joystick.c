#include "hw.h"
#include "joystick.h"

#define JOYSTICK_SW_PIN 0U
#define ADC_LOW  1300U
#define ADC_HIGH 2800U

static uint16_t adc_read(uint32_t channel)
{
    ADC1->SQR1 = (channel & 0x1FU) << 6;
    ADC1->ISR = 1UL << 3;
    ADC1->CR |= 1UL << 2;
    while ((ADC1->ISR & (1UL << 2)) == 0U)
    {
    }
    return (uint16_t)ADC1->DR;
}

void joystick_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2_GPIOAEN | RCC_AHB2_GPIOBEN |
                    RCC_AHB2_GPIOCEN | RCC_AHB2_ADCEN;
    gpio_set_analog(GPIOA, 0U);
    gpio_set_analog(GPIOA, 1U);
    gpio_set_input_pullup(GPIOB, JOYSTICK_SW_PIN);

    ADC1->CR |= 1UL << 28;
    delay_ms(1U);
    ADC1->CR |= 1UL << 31;
    while ((ADC1->CR & (1UL << 31)) != 0U)
    {
    }
    ADC1->SMPR1 |= (7UL << 15) | (7UL << 18);
    ADC1->CFGR = 0U;
    ADC1->CR |= 1UL;
    while ((ADC1->ISR & 1UL) == 0U)
    {
    }
}

JoystickState joystick_read(void)
{
    JoystickState state;
    uint32_t x_left;
    uint32_t x_right;
    uint32_t y_up;
    uint32_t y_down;

    state.x = adc_read(5U);
    state.y = adc_read(6U);
    state.pressed = (GPIOB->IDR & (1UL << JOYSTICK_SW_PIN)) == 0U;

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
