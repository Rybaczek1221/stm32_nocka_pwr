#include "hw.h"
#include "game.h"
#include "joystick.h"
#include "led_matrix.h"
#include "tft_st7735.h"

static void board_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2_GPIOAEN | RCC_AHB2_GPIOBEN |
                    RCC_AHB2_GPIOCEN;
    gpio_set_alternate5(GPIOB, 10U); /* SPI2_SCK. */
    gpio_set_alternate5(GPIOC, 3U);  /* SPI2_MOSI. */
}

static uint32_t joystick_button_event(uint8_t pressed)
{
    static uint8_t previous_pressed;
    uint32_t event = pressed != 0U && previous_pressed == 0U;
    previous_pressed = pressed;
    return event;
}

int main(void)
{
    JoystickState joystick;
    board_init();
    led_matrix_init();
    led_matrix_startup_signal();
    tft_init();
    joystick_init();
    game_init();

    for (;;)
    {
        joystick = joystick_read();
        led_matrix_show(joystick.direction);
        game_update(joystick, joystick_button_event(joystick.pressed));
        delay_ms(30U);
    }
}
