#include "game.h"
#include "tft_st7735.h"

static uint16_t player_x;
static uint16_t player_y;
static uint16_t previous_x;
static uint16_t previous_y;

void game_init(void)
{
    player_x = TFT_WIDTH / 2U;
    player_y = TFT_HEIGHT / 2U;
    previous_x = player_x;
    previous_y = player_y;
    tft_fill_screen(TFT_BLACK);
    tft_fill_rect(4U, 4U, 120U, 152U, TFT_BLUE);
    tft_fill_rect(6U, 6U, 116U, 148U, TFT_BLACK);
    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
}

void game_update(JoystickState state)
{
    tft_fill_rect(previous_x - 2U, previous_y - 2U, 5U, 5U, TFT_BLACK);

    if (state.direction == JOY_UP || state.direction == JOY_UP_LEFT ||
        state.direction == JOY_UP_RIGHT)
    {
        if (player_y > 9U) --player_y;
    }
    if (state.direction == JOY_DOWN || state.direction == JOY_DOWN_LEFT ||
        state.direction == JOY_DOWN_RIGHT)
    {
        if (player_y < TFT_HEIGHT - 10U) ++player_y;
    }
    if (state.direction == JOY_LEFT || state.direction == JOY_UP_LEFT ||
        state.direction == JOY_DOWN_LEFT)
    {
        if (player_x > 9U) --player_x;
    }
    if (state.direction == JOY_RIGHT || state.direction == JOY_UP_RIGHT ||
        state.direction == JOY_DOWN_RIGHT)
    {
        if (player_x < TFT_WIDTH - 10U) ++player_x;
    }

    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
    previous_x = player_x;
    previous_y = player_y;
}
