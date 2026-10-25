#include "hw.h"
#include "led_matrix.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint8_t pin;
} LedPin;

static const LedPin leds[9] =
{
    {GPIOB, 1U}, {GPIOB, 2U}, {GPIOB, 10U},
    {GPIOB, 11U}, {GPIOB, 12U}, {GPIOB, 13U},
    {GPIOB, 14U}, {GPIOB, 15U}, {GPIOC, 6U}
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
