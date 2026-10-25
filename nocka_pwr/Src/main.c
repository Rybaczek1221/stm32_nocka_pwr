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

int main(void)
{
    JoystickState joystick;

    board_init();
    joystick_init();
    led_matrix_init();
    tft_init();
    game_init();

    for (;;)
    {
        joystick = joystick_read();
        led_matrix_show(joystick.direction);
        game_update(joystick);
        delay_ms(30U);
    }
}
