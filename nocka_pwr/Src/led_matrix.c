#include "hw.h"
#include "led_matrix.h"
#include <stdint.h>

typedef struct
{
    GPIO_TypeDef *port;
    uint8_t pin;
} LedPin;

static const LedPin leds[9] =
{
    {GPIOB, 1U}, {GPIOB, 6U}, {GPIOB, 7U},
    {GPIOB, 8U}, {GPIOB, 9U}, {GPIOC, 6U},
    {GPIOC, 7U}, {GPIOC, 8U}, {GPIOC, 9U}
};

void led_matrix_init(void)
{
    uint32_t index;

    for (index = 0U; index < 9U; ++index)
    {
        gpio_set_output(leds[index].port, leds[index].pin);
        gpio_write(leds[index].port, leds[index].pin, 0U);
    }
}

void led_matrix_show(JoystickDirection direction)
{
    static const uint8_t direction_to_led[9] =
    {
        4U, 1U, 2U, 5U, 8U, 7U, 6U, 3U, 0U
    };
    uint32_t index;

    for (index = 0U; index < 9U; ++index)
    {
        gpio_write(leds[index].port, leds[index].pin,
                   index == direction_to_led[direction]);
    }
}

void led_matrix_all(uint32_t on)
{
    uint32_t index;

    for (index = 0U; index < 9U; ++index)
    {
        gpio_write(leds[index].port, leds[index].pin, on);
    }
}

void led_matrix_startup_signal(void)
{
    static const JoystickDirection sequence[] =
    {
        JOY_UP_LEFT, JOY_UP, JOY_UP_RIGHT,
        JOY_RIGHT, JOY_DOWN_RIGHT, JOY_DOWN,
        JOY_DOWN_LEFT, JOY_LEFT, JOY_CENTER
    };
    uint32_t index;

    led_matrix_all(1U);
    delay_ms(250U);
    led_matrix_all(0U);
    delay_ms(150U);

    for (index = 0U; index < 9U; ++index)
    {
        led_matrix_show(sequence[index]);
        delay_ms(80U);
    }

    led_matrix_all(0U);
}
