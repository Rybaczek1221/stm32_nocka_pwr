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

static uint32_t joystick_button_pressed(void)
{
    return (GPIOB->IDR & (1UL << 0U)) == 0U;
}

static uint32_t joystick_button_event(void)
{
    if (!joystick_button_pressed())
    {
        return 0U;
    }

    /* Confirm the press and consume the whole press-release action. */
    delay_ms(25U);
    if (!joystick_button_pressed())
    {
        return 0U;
    }

    while (joystick_button_pressed())
    {
    }
    delay_ms(25U);
    return 1U;
}

int main(void)
{
    JoystickState joystick;
    uint32_t game_started = 0U;

    board_init();
    led_matrix_init();
    led_matrix_startup_signal();
    tft_init();
    /* Visible hardware checkpoint: before pressing SW the whole TFT is blue. */
    tft_fill_screen(TFT_BLUE);
    joystick_init();

    for (;;)
    {
        joystick = joystick_read();

        if (joystick_button_event())
        {
            game_started = 1U;
            game_init();
        }

        if (game_started != 0U)
        {
            led_matrix_show(joystick.direction);
            game_update(joystick);
        }

        delay_ms(30U);
    }
}
