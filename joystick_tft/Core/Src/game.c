#include "game.h"
#include "tft_st7735.h"

static uint16_t player_x;
static uint16_t player_y;
static uint16_t previous_x;
static uint16_t previous_y;

void game_show_menu(void)
{
    tft_fill_screen(TFT_BLACK);
    tft_fill_rect(8U, 8U, 144U, 24U, TFT_BLUE);
    tft_fill_rect(16U, 48U, 128U, 20U, TFT_YELLOW);
    tft_fill_rect(24U, 52U, 112U, 12U, TFT_BLACK);
    tft_fill_rect(28U, 88U, 104U, 16U, TFT_BLUE);
    tft_fill_rect(40U, 92U, 80U, 8U, TFT_BLACK);
}

void game_start(void)
{
    player_x = TFT_WIDTH / 2U;
    player_y = TFT_HEIGHT / 2U;
    previous_x = player_x;
    previous_y = player_y;

    tft_fill_screen(TFT_BLACK);
    tft_fill_rect(2U, 2U, TFT_WIDTH - 4U, TFT_HEIGHT - 4U, TFT_BLUE);
    tft_fill_rect(4U, 4U, TFT_WIDTH - 8U, TFT_HEIGHT - 8U, TFT_BLACK);
    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
}

void game_update(JoystickState state)
{
    if (state.direction == JOY_CENTER)
    {
        return;
    }

    tft_fill_rect(previous_x - 2U, previous_y - 2U, 5U, 5U, TFT_BLACK);

    if (state.direction == JOY_UP || state.direction == JOY_UP_LEFT ||
        state.direction == JOY_UP_RIGHT)
    {
        if (player_y > 8U) --player_y;
    }
    if (state.direction == JOY_DOWN || state.direction == JOY_DOWN_LEFT ||
        state.direction == JOY_DOWN_RIGHT)
    {
        if (player_y < TFT_HEIGHT - 9U) ++player_y;
    }
    if (state.direction == JOY_LEFT || state.direction == JOY_UP_LEFT ||
        state.direction == JOY_DOWN_LEFT)
    {
        if (player_x > 8U) --player_x;
    }
    if (state.direction == JOY_RIGHT || state.direction == JOY_UP_RIGHT ||
        state.direction == JOY_DOWN_RIGHT)
    {
        if (player_x < TFT_WIDTH - 9U) ++player_x;
    }

    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
    previous_x = player_x;
    previous_y = player_y;
}
