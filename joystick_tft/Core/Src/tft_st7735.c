#include "tft_st7735.h"
#include "spi.h"

#define TFT_X_OFFSET 1U
#define TFT_Y_OFFSET 2U

#define TFT_CS_LOW()  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define TFT_CS_HIGH() HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define TFT_DC_COMMAND() HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define TFT_DC_DATA()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

static void tft_write(const uint8_t *data, uint16_t length)
{
    if (HAL_SPI_Transmit(&hspi2, (uint8_t *)data, length, HAL_MAX_DELAY) != HAL_OK)
    {
        Error_Handler();
    }
}

static void tft_command(uint8_t command)
{
    TFT_DC_COMMAND();
    TFT_CS_LOW();
    tft_write(&command, 1U);
    TFT_CS_HIGH();
}

static void tft_data(const uint8_t *data, uint16_t length)
{
    TFT_DC_DATA();
    TFT_CS_LOW();
    tft_write(data, length);
    TFT_CS_HIGH();
}

static void tft_data16(uint16_t value)
{
    uint8_t data[2] = {(uint8_t)(value >> 8), (uint8_t)value};
    tft_data(data, sizeof(data));
}

static void tft_set_window(uint16_t x, uint16_t y, uint16_t width,
                           uint16_t height)
{
    tft_command(0x2AU);
    tft_data16(TFT_X_OFFSET + x);
    tft_data16(TFT_X_OFFSET + x + width - 1U);

    tft_command(0x2BU);
    tft_data16(TFT_Y_OFFSET + y);
    tft_data16(TFT_Y_OFFSET + y + height - 1U);

    tft_command(0x2CU);
}

void tft_init(void)
{
    static const uint8_t frame_rate[] = {0x01U, 0x2CU, 0x2DU};
    static const uint8_t frame_rate_3[] =
        {0x01U, 0x2CU, 0x2DU, 0x01U, 0x2CU, 0x2DU};
    static const uint8_t gamma_positive[] =
        {0x0FU, 0x1AU, 0x0FU, 0x18U, 0x2FU, 0x28U, 0x20U, 0x22U,
         0x1FU, 0x1BU, 0x23U, 0x37U, 0x00U, 0x07U, 0x02U, 0x10U};
    static const uint8_t gamma_negative[] =
        {0x0FU, 0x1BU, 0x0FU, 0x17U, 0x33U, 0x2CU, 0x29U, 0x2EU,
         0x30U, 0x30U, 0x39U, 0x3FU, 0x00U, 0x07U, 0x03U, 0x10U};
    static const uint8_t power_1[] = {0xA2U, 0x02U, 0x84U};
    static const uint8_t power_3[] = {0x0AU, 0x00U};
    static const uint8_t power_4[] = {0x8AU, 0x2AU};
    static const uint8_t power_5[] = {0x8AU, 0xEEU};

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100U);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100U);

    tft_command(0x01U);
    HAL_Delay(150U);

    tft_command(0xB1U); tft_data(frame_rate, sizeof(frame_rate));
    tft_command(0xB2U); tft_data(frame_rate, sizeof(frame_rate));
    tft_command(0xB3U); tft_data(frame_rate_3, sizeof(frame_rate_3));
    tft_command(0xB4U);
    { const uint8_t value = 0x07U; tft_data(&value, 1U); }
    tft_command(0xC0U); tft_data(power_1, sizeof(power_1));
    tft_command(0xC1U);
    { const uint8_t value = 0xC5U; tft_data(&value, 1U); }
    tft_command(0xC2U); tft_data(power_3, sizeof(power_3));
    tft_command(0xC3U); tft_data(power_4, sizeof(power_4));
    tft_command(0xC4U); tft_data(power_5, sizeof(power_5));
    tft_command(0xC5U);
    { const uint8_t value = 0x0EU; tft_data(&value, 1U); }
    tft_command(0xE0U); tft_data(gamma_positive, sizeof(gamma_positive));
    tft_command(0xE1U); tft_data(gamma_negative, sizeof(gamma_negative));
    tft_command(0xF0U);
    { const uint8_t value = 0x01U; tft_data(&value, 1U); }
    tft_command(0xF6U);
    { const uint8_t value = 0x00U; tft_data(&value, 1U); }
    tft_command(0x3AU);
    { const uint8_t value = 0x05U; tft_data(&value, 1U); }
    tft_command(0x36U);
    { const uint8_t value = 0xA0U; tft_data(&value, 1U); }

    tft_command(0x11U);
    HAL_Delay(120U);
    tft_command(0x29U);
    HAL_Delay(20U);
}

void tft_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   uint16_t color)
{
    /* Send one complete scanline per HAL transaction instead of one pixel. */
    uint8_t line[2U * TFT_WIDTH];
    uint16_t column;
    uint16_t row;

    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || width == 0U || height == 0U)
    {
        return;
    }
    if ((uint32_t)x + width > TFT_WIDTH) width = TFT_WIDTH - x;
    if ((uint32_t)y + height > TFT_HEIGHT) height = TFT_HEIGHT - y;

    for (column = 0U; column < width; ++column)
    {
        line[2U * column] = (uint8_t)(color >> 8);
        line[2U * column + 1U] = (uint8_t)color;
    }
    tft_set_window(x, y, width, height);
    TFT_DC_DATA();
    TFT_CS_LOW();
    for (row = 0U; row < height; ++row)
    {
        tft_write(line, (uint16_t)(2U * width));
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
    if (character >= 'A' && character <= 'Z') return (uint8_t)(character - 'A' + 1);
    if (character >= '0' && character <= '9') return (uint8_t)(character - '0' + 27);
    if (character == ':') return 36U;
    return 0U;
}

void tft_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color,
                   uint16_t background, uint16_t scale)
{
    uint16_t cursor = x;
    uint8_t pixels[10U * 14U * 2U];
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
        for (column = 0U; column < 5U * scale; ++column)
        {
            for (row = 0U; row < 7U * scale; ++row)
            {
                uint16_t pixel = (font[index][column / scale] &
                                  (1U << (row / scale))) ?
                                 color : background;
                uint16_t offset = (uint16_t)(2U * (row * 5U * scale + column));
                pixels[offset] = (uint8_t)(pixel >> 8);
                pixels[offset + 1U] = (uint8_t)pixel;
            }
        }
        tft_set_window(cursor, y, (uint16_t)(5U * scale),
                       (uint16_t)(7U * scale));
        TFT_DC_DATA();
        TFT_CS_LOW();
        tft_write(pixels, (uint16_t)(2U * 5U * 7U * scale * scale));
        TFT_CS_HIGH();
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
