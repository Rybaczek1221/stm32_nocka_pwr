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
    uint8_t data[2] = {(uint8_t)(color >> 8), (uint8_t)color};
    uint32_t pixels;

    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || width == 0U || height == 0U)
    {
        return;
    }
    if ((uint32_t)x + width > TFT_WIDTH) width = TFT_WIDTH - x;
    if ((uint32_t)y + height > TFT_HEIGHT) height = TFT_HEIGHT - y;

    tft_set_window(x, y, width, height);
    TFT_DC_DATA();
    TFT_CS_LOW();
    pixels = (uint32_t)width * height;
    while (pixels-- != 0U)
    {
        tft_write(data, sizeof(data));
    }
    TFT_CS_HIGH();
}

void tft_fill_screen(uint16_t color)
{
    tft_fill_rect(0U, 0U, TFT_WIDTH, TFT_HEIGHT, color);
}
