#include "hw.h"
#include "tft_st7735.h"
#include <stddef.h>

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
    while ((SPI2->SR & (1UL << 7)) != 0U)
    {
    }
    while ((SPI2->SR & (1UL << 1)) == 0U)
    {
    }
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
    const uint16_t x_start = x + 1U;
    const uint16_t y_start = y + 2U;

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
    static const uint16_t init_table[] =
    {
        0x1B1U, 0x01U, 0x2CU, 0x2DU,
        0x1B2U, 0x01U, 0x2CU, 0x2DU,
        0x1B3U, 0x01U, 0x2CU, 0x2DU, 0x01U, 0x2CU, 0x2DU,
        0x1B4U, 0x07U,
        0x1C0U, 0xA2U, 0x02U, 0x84U,
        0x1C1U, 0xC5U,
        0x1C2U, 0x0AU, 0x00U,
        0x1C3U, 0x8AU, 0x2AU,
        0x1C4U, 0x8AU, 0xEEU,
        0x1C5U, 0x0EU,
        0x1E0U, 0x0FU, 0x1AU, 0x0FU, 0x18U, 0x2FU, 0x28U, 0x20U,
        0x22U, 0x1FU, 0x1BU, 0x23U, 0x37U, 0x00U, 0x07U, 0x02U, 0x10U,
        0x1E1U, 0x0FU, 0x1BU, 0x0FU, 0x17U, 0x33U, 0x2CU, 0x29U,
        0x2EU, 0x30U, 0x30U, 0x39U, 0x3FU, 0x00U, 0x07U, 0x03U, 0x10U,
        0x1F0U, 0x01U,
        0x1F6U, 0x00U,
        0x13AU, 0x05U,
        0x136U, 0xA0U
    };
    uint32_t index;

    gpio_set_output(GPIOB, TFT_CS_PIN);
    gpio_set_output(GPIOB, TFT_DC_PIN);
    gpio_set_output(GPIOB, TFT_RST_PIN);
    TFT_CS_HIGH();
    TFT_DC_DATA();
    gpio_write(GPIOB, TFT_RST_PIN, 1U);

    RCC->APB1ENR1 |= RCC_APB1_SPI2EN;
    SPI2->CR1 = 0U;
    /*
     * STM32L4 SPI data size is configured in CR2. Set an explicit 8-bit
     * frame and FIFO threshold so the driver does not depend on reset state.
     */
    SPI2->CR2 = (7UL << 8) | (1UL << 12);
    SPI2->CR1 = (1UL << 2)       /* Master mode. */
              | (2UL << 3)       /* HSI16 / 8 = about 2 MHz. */
              | (1UL << 14)      /* Transmit direction in 1-line mode. */
              | (1UL << 15)      /* 1-line SPI transmit mode. */
              | (1UL << 8)       /* Software NSS management. */
              | (1UL << 9);      /* Internal NSS high. */
    SPI2->CR1 |= 1UL << 6;        /* Enable SPI2. */

    gpio_write(GPIOB, TFT_RST_PIN, 0U);
    delay_ms(100U);
    gpio_write(GPIOB, TFT_RST_PIN, 1U);
    delay_ms(100U);

    for (index = 0U; index < sizeof(init_table) / sizeof(init_table[0]);
         ++index)
    {
        if ((init_table[index] & 0x100U) != 0U)
        {
            tft_command((uint8_t)init_table[index]);
        }
        else
        {
            tft_data((const uint8_t *)&init_table[index], 1U);
        }
    }

    delay_ms(200U);
    tft_command(0x11U);
    delay_ms(120U);
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

static const uint8_t font[37][5] =
{
    {0, 0, 0, 0, 0},
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0, 0x41, 0x7F, 0x41, 0}, {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
    {0x3E, 0x45, 0x49, 0x51, 0x3E}, {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03}, {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x14, 0x08, 0x3E, 0x08, 0x14}
};

static uint8_t font_index(char character)
{
    if (character >= 'A' && character <= 'Z')
    {
        return (uint8_t)(character - 'A' + 1);
    }
    if (character >= '0' && character <= '9')
    {
        return (uint8_t)(character - '0' + 27);
    }
    if (character == ':')
    {
        return 36U;
    }
    return 0U;
}

void tft_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color,
                   uint16_t background, uint16_t scale)
{
    uint16_t cursor = x;
    uint8_t column;
    uint8_t row;
    uint8_t index;

    if (scale == 0U) scale = 1U;
    while (*text != '\0')
    {
        if (*text == '\n')
        {
            cursor = x;
            y = (uint16_t)(y + 8U * scale);
            ++text;
            continue;
        }
        index = font_index(*text++);
        for (column = 0U; column < 5U; ++column)
        {
            for (row = 0U; row < 7U; ++row)
            {
                uint16_t pixel = (font[index][column] & (1U << row)) ?
                                 color : background;
                tft_fill_rect(cursor + column * scale, y + row * scale,
                              scale, scale, pixel);
            }
        }
        cursor = (uint16_t)(cursor + 6U * scale);
    }
}

void tft_draw_uint(uint16_t x, uint16_t y, uint32_t value, uint16_t color,
                   uint16_t background, uint16_t scale)
{
    char digits[11];
    uint8_t position = sizeof(digits) - 1U;

    digits[position] = '\0';
    do
    {
        digits[--position] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U && position != 0U);
    tft_draw_text(x, y, &digits[position], color, background, scale);
}
