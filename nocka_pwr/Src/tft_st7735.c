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
    const uint16_t x_start = x + 2U;
    const uint16_t y_start = y + 1U;

    tft_command(0x2AU);
    data[0] = (uint8_t)(x_start >> 8);
    data[1] = (uint8_t)x_start;
    data[2] = (uint8_t)((x_start + width - 1U) >> 8);
    data[3] = (uint8_t)(x_start + width - 1U);
    tft_data(data, 4U);

    tft_command(0x2BU);
    data[0] = (uint8_t)(y_start >> 8);
    data[1] = (uint8_t)y_start;
    data[2] = (uint8_t)((y_start + height - 1U) >> 8);
    data[3] = (uint8_t)(y_start + height - 1U);
    tft_data(data, 4U);

    tft_command(0x2CU);
}

void tft_init(void)
{
    static const uint8_t frame_rate[] = {0x01U, 0x2CU, 0x2DU};
    static const uint8_t frame_rate_3[] =
        {0x01U, 0x2CU, 0x2DU, 0x01U, 0x2CU, 0x2DU};
    static const uint8_t power_1[] = {0xA2U, 0x02U, 0x84U};
    static const uint8_t power_3[] = {0x0AU, 0x00U};
    static const uint8_t power_4[] = {0x8AU, 0x2AU};
    static const uint8_t power_5[] = {0x8AU, 0xEEU};
    static const uint8_t gamma_positive[] =
        {0x02U, 0x1CU, 0x07U, 0x12U, 0x37U, 0x32U, 0x29U, 0x2DU,
         0x29U, 0x25U, 0x2BU, 0x39U, 0x00U, 0x01U, 0x03U, 0x10U};
    static const uint8_t gamma_negative[] =
        {0x03U, 0x1DU, 0x07U, 0x06U, 0x2EU, 0x2CU, 0x29U, 0x2DU,
         0x2EU, 0x2EU, 0x37U, 0x3FU, 0x00U, 0x00U, 0x02U, 0x10U};
    static const uint8_t color_mode = 0x05U;
    static const uint8_t orientation = 0xC8U;
    static const uint8_t column_range[] = {0x00U, 0x02U, 0x00U, 0x81U};
    static const uint8_t row_range[] = {0x00U, 0x01U, 0x00U, 0xA0U};

    gpio_set_output(GPIOB, TFT_CS_PIN);
    gpio_set_output(GPIOB, TFT_DC_PIN);
    gpio_set_output(GPIOB, TFT_RST_PIN);
    TFT_CS_HIGH();
    TFT_DC_DATA();
    gpio_write(GPIOB, TFT_RST_PIN, 1U);

    RCC->APB1ENR1 |= RCC_APB1_SPI2EN;
    SPI2->CR1 = 0U;
    SPI2->CR1 = (1UL << 2)       /* Master mode. */
              | (2UL << 3)       /* HSI16 / 8 = about 2 MHz. */
              | (1UL << 8)       /* Software NSS management. */
              | (1UL << 9);      /* Internal NSS high. */
    SPI2->CR1 |= 1UL << 6;        /* Enable SPI2. */

    gpio_write(GPIOB, TFT_RST_PIN, 0U);
    delay_ms(20U);
    gpio_write(GPIOB, TFT_RST_PIN, 1U);
    delay_ms(120U);

    tft_command(0x01U);
    delay_ms(150U);
    tft_command(0x11U);
    delay_ms(120U);

    tft_command(0xB1U);
    tft_data(frame_rate, sizeof(frame_rate));
    tft_command(0xB2U);
    tft_data(frame_rate, sizeof(frame_rate));
    tft_command(0xB3U);
    tft_data(frame_rate_3, sizeof(frame_rate_3));
    tft_command(0xB4U);
    {
        static const uint8_t value = 0x07U;
        tft_data(&value, 1U);
    }
    tft_command(0xC0U);
    tft_data(power_1, sizeof(power_1));
    tft_command(0xC1U);
    {
        static const uint8_t value = 0xC5U;
        tft_data(&value, 1U);
    }
    tft_command(0xC2U);
    tft_data(power_3, sizeof(power_3));
    tft_command(0xC3U);
    tft_data(power_4, sizeof(power_4));
    tft_command(0xC4U);
    tft_data(power_5, sizeof(power_5));
    tft_command(0xC5U);
    {
        static const uint8_t value = 0x0EU;
        tft_data(&value, 1U);
    }
    tft_command(0x20U);
    tft_command(0x36U);
    tft_data(&orientation, 1U);
    tft_command(0x3AU);
    tft_data(&color_mode, 1U);
    tft_command(0xE0U);
    tft_data(gamma_positive, sizeof(gamma_positive));
    tft_command(0xE1U);
    tft_data(gamma_negative, sizeof(gamma_negative));
    tft_command(0x2AU);
    tft_data(column_range, sizeof(column_range));
    tft_command(0x2BU);
    tft_data(row_range, sizeof(row_range));
    tft_command(0x13U);
    tft_command(0x29U);
    delay_ms(100U);
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
