#include "lives.h"

#define LIFE_COUNT 5U

static const uint16_t life_pins[LIFE_COUNT] =
{
    GPIO_PIN_6, GPIO_PIN_7, GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10
};
static uint8_t lives_remaining;

static void lives_draw(void)
{
    uint8_t index;
    for (index = 0U; index < LIFE_COUNT; ++index)
    {
        HAL_GPIO_WritePin(GPIOC, life_pins[index],
                          index >= LIFE_COUNT - lives_remaining ? GPIO_PIN_SET :
                          GPIO_PIN_RESET);
    }
}

void lives_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    lives_reset();
}

void lives_reset(void)
{
    lives_remaining = LIFE_COUNT;
    lives_draw();
}

uint8_t lives_lose(void)
{
    uint8_t index;
    if (lives_remaining == 0U) return 0U;
    --lives_remaining;
    lives_draw();
    if (lives_remaining != 0U) return 1U;

    for (index = 0U; index < 3U; ++index)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 |
                          GPIO_PIN_9 | GPIO_PIN_10, GPIO_PIN_SET);
        HAL_Delay(80U);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 |
                          GPIO_PIN_9 | GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_Delay(80U);
    }
    return 0U;
}

uint8_t lives_get(void)
{
    return lives_remaining;
}

void lives_set_led(uint8_t index, uint8_t enabled)
{
    if (index >= LIFE_COUNT) return;
    HAL_GPIO_WritePin(GPIOC, life_pins[index],
                      enabled != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
