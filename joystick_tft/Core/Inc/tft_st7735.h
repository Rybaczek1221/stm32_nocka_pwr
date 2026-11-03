#ifndef TFT_ST7735_H
#define TFT_ST7735_H

#include "main.h"

#define TFT_WIDTH  160U
#define TFT_HEIGHT 128U

#define TFT_BLACK  0x0000U
#define TFT_BLUE   0x001FU
#define TFT_YELLOW 0xFFE0U
#define TFT_WHITE  0xFFFFU
#define TFT_RED    0xF800U
#define TFT_GREEN  0x07E0U
#define TFT_CYAN   0x07FFU
#define TFT_NAVY   0x1108U
#define TFT_PANEL  0x2108U
#define TFT_PURPLE 0x780FU
#define TFT_VIOLET 0xA81FU
#define TFT_GRID   0x528AU
#define TFT_ORANGE 0xFD20U

void tft_init(void);
void tft_fill_screen(uint16_t color);
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   uint16_t color);
void tft_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color,
                   uint16_t background, uint16_t scale);
void tft_draw_uint(uint16_t x, uint16_t y, uint32_t value, uint16_t color,
                   uint16_t background, uint16_t scale);

#endif
