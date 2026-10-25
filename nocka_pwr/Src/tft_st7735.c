#include "hw.h"
#include "tft_st7735.h"

#define TFT_CS_PIN 12U
#define TFT_DC_PIN 11U
#define TFT_RST_PIN 2U

#define TFT_CS_LOW()  gpio_write(GPIOB, TFT_CS_PIN, 0U)
#define TFT_CS_HIGH() gpio_write(GPIOB, TFT_CS_PIN, 1U)
#define TFT_DC_CMD()  gpio_write(GPIOB, TFT_DC_PIN, 0U)
#define TFT_DC_DATA() gpio_write(GPIOB, TFT_DC_PIN, 1U)

static void spi_write(uint8_t value)
{
    while ((SPI2->SR & (1UL << 1)) == 0U)
    {
    }
    SPI2->DR = value;
    while ((SPI2->SR & (1UL << 0)) == 0U)
    {
    }
    (void)SPI2->DR;
}

static void tft_command(uint8_t command)
{
    TFT_DC_CMD();
    TFT_CS_LOW();
    spi_write(command);
    TFT_CS_HIGH();
}

static void tft_data(const uint8_t *data, uint32_t length)
{
    TFT_DC_DATA();
    TFT_CS_LOW();
    while (length-- != 0U)
    {
        spi_write(*data++);
    }
    TFT_CS_HIGH();
}

static void tft_set_window(uint16_t x, uint16_t y, uint16_t width,
                           uint16_t height)
{
    uint8_t data[4];

    tft_command(0x2AU);
    data[0] = (uint8_t)(x >> 8);
    data[1] = (uint8_t)x;
    data[2] = (uint8_t)((x + width - 1U) >> 8);
    data[3] = (uint8_t)(x + width - 1U);
    tft_data(data, 4U);

    tft_command(0x2BU);
    data[0] = (uint8_t)(y >> 8);
    data[1] = (uint8_t)y;
    data[2] = (uint8_t)((y + height - 1U) >> 8);
    data[3] = (uint8_t)(y + height - 1U);
    tft_data(data, 4U);

    tft_command(0x2CU);
}

void tft_init(void)
{
    uint8_t color_mode = 0x05U;
    uint8_t orientation = 0x00U;

    gpio_set_output(GPIOB, TFT_CS_PIN);
    gpio_set_output(GPIOB, TFT_DC_PIN);
    gpio_set_output(GPIOB, TFT_RST_PIN);
    TFT_CS_HIGH();
    TFT_DC_DATA();
    gpio_write(GPIOB, TFT_RST_PIN, 1U);

    RCC->APB1ENR1 |= RCC_APB1_SPI2EN;
    SPI2->CR1 = (1UL << 2) | (1UL << 8) | (1UL << 9);
    SPI2->CR1 |= 2UL << 3; /* 16 MHz / 8 = 2 MHz during first test. */
    SPI2->CR1 |= 1UL << 6;

    gpio_write(GPIOB, TFT_RST_PIN, 0U);
    delay_ms(20U);
    gpio_write(GPIOB, TFT_RST_PIN, 1U);
    delay_ms(120U);

    tft_command(0x01U);
    delay_ms(150U);
    tft_command(0x11U);
    delay_ms(120U);
    tft_command(0x3AU);
    tft_data(&color_mode, 1U);
    tft_command(0x36U);
    tft_data(&orientation, 1U);
    tft_command(0x13U);
    tft_command(0x29U);
    delay_ms(20U);
}

void tft_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   uint16_t color)
{
    uint32_t pixels;
    uint8_t data[2];

    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || width == 0U || height == 0U)
    {
        return;
    }
    if ((uint32_t)x + width > TFT_WIDTH)
    {
        width = TFT_WIDTH - x;
    }
    if ((uint32_t)y + height > TFT_HEIGHT)
    {
        height = TFT_HEIGHT - y;
    }

    tft_set_window(x, y, width, height);
    data[0] = (uint8_t)(color >> 8);
    data[1] = (uint8_t)color;
    TFT_DC_DATA();
    TFT_CS_LOW();
    pixels = (uint32_t)width * height;
    while (pixels-- != 0U)
    {
        spi_write(data[0]);
        spi_write(data[1]);
    }
    TFT_CS_HIGH();
}

void tft_fill_screen(uint16_t color)
{
    tft_fill_rect(0U, 0U, TFT_WIDTH, TFT_HEIGHT, color);
}
