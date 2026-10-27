#ifndef TFT_ST7735_H
#define TFT_ST7735_H

#include "main.h"

#define TFT_WIDTH  160U
#define TFT_HEIGHT 128U

#define TFT_BLACK  0x0000U
#define TFT_BLUE   0x001FU
#define TFT_YELLOW 0xFFE0U

void tft_init(void);
void tft_fill_screen(uint16_t color);
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   uint16_t color);

#endif
