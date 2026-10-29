#include "game.h"
#include "tft_st7735.h"

#define PLAY_TOP 18U
#define PLAY_LEFT 4U
#define PLAY_RIGHT 155U
#define PLAY_BOTTOM 123U
#define SNAKE_CELL 6U
#define SNAKE_COLS 25U
#define SNAKE_ROWS 17U
#define MAX_SNAKE 60U
#define MAX_BOMBS 10U

typedef struct
{
    uint16_t x;
    uint16_t y;
} Position;

static GameScreen screen;
static GameScreen paused_screen;
static uint8_t menu_item;
static uint8_t pause_item;
static JoystickDirection last_menu_direction;
static uint32_t random_state;
static uint32_t frame;
static uint32_t score;

static Position snake[MAX_SNAKE];
static uint8_t snake_length;
static Position food;
static JoystickDirection snake_direction;
static Position bombs[MAX_BOMBS];
static uint8_t bomb_count;
static uint16_t player_x;
static uint16_t player_y;

static uint32_t next_random(void)
{
    random_state = random_state * 1664525UL + 1013904223UL;
    return random_state;
}

static void draw_header(const char *title)
{
    tft_fill_screen(TFT_BLACK);
    tft_draw_text(4U, 2U, title, TFT_CYAN, TFT_BLACK, 1U);
}

static void draw_menu(void)
{
    tft_fill_screen(TFT_BLACK);
    tft_draw_text(20U, 5U, "JAROSLAW RYBAK", TFT_YELLOW, TFT_BLACK, 1U);
    tft_draw_text(48U, 14U, "PWR", TFT_CYAN, TFT_BLACK, 1U);
    tft_draw_text(16U, 31U, "WYBIERZ GRE", TFT_WHITE, TFT_BLACK, 1U);
    tft_draw_text(29U, 52U, menu_item == 0U ? "> SNAKE" : "  SNAKE",
                  menu_item == 0U ? TFT_YELLOW : TFT_WHITE, TFT_BLACK, 2U);
    tft_draw_text(29U, 75U, menu_item == 1U ? "> KROPKA" : "  KROPKA",
                  menu_item == 1U ? TFT_YELLOW : TFT_WHITE, TFT_BLACK, 2U);
    tft_draw_text(10U, 111U, "SW: START", TFT_GREEN, TFT_BLACK, 1U);
}

static void draw_pause(void)
{
    tft_fill_rect(15U, 35U, 130U, 58U, TFT_BLUE);
    tft_draw_text(48U, 40U, "PAUZA", TFT_YELLOW, TFT_BLUE, 1U);
    tft_draw_text(28U, 56U, pause_item == 0U ? "> WZNOW" : "  WZNOW",
                  pause_item == 0U ? TFT_WHITE : TFT_CYAN, TFT_BLUE, 1U);
    tft_draw_text(28U, 72U, pause_item == 1U ? "> MENU" : "  MENU",
                  pause_item == 1U ? TFT_WHITE : TFT_CYAN, TFT_BLUE, 1U);
}

static void draw_game_over(void)
{
    tft_fill_screen(TFT_BLACK);
    tft_draw_text(25U, 32U, "KONIEC GRY", TFT_RED, TFT_BLACK, 1U);
    tft_draw_text(31U, 52U, "WYNIK:", TFT_WHITE, TFT_BLACK, 1U);
    tft_draw_uint(75U, 52U, score, TFT_YELLOW, TFT_BLACK, 1U);
    tft_draw_text(22U, 83U, "SW: MENU", TFT_GREEN, TFT_BLACK, 1U);
}

static void start_snake(uint32_t seed)
{
    uint8_t index;
    screen = GAME_SNAKE;
    random_state = seed ^ 0xA53C9E17UL;
    frame = 0U;
    score = 0U;
    snake_length = 4U;
    snake_direction = JOY_RIGHT;
    for (index = 0U; index < snake_length; ++index)
    {
        snake[index].x = 10U - index;
        snake[index].y = 8U;
    }
    food.x = (uint16_t)(next_random() % SNAKE_COLS);
    food.y = (uint16_t)(next_random() % SNAKE_ROWS);
    draw_header("SNAKE  WYNIK: 0");
    tft_draw_text(4U, 11U, "JOYSTICK: RUCH  SW: PAUZA", TFT_WHITE,
                  TFT_BLACK, 1U);
    for (index = 0U; index < snake_length; ++index)
    {
        tft_fill_rect(PLAY_LEFT + snake[index].x * SNAKE_CELL,
                      PLAY_TOP + snake[index].y * SNAKE_CELL,
                      SNAKE_CELL - 1U, SNAKE_CELL - 1U, TFT_GREEN);
    }
    tft_fill_rect(PLAY_LEFT + food.x * SNAKE_CELL,
                  PLAY_TOP + food.y * SNAKE_CELL, SNAKE_CELL - 1U,
                  SNAKE_CELL - 1U, TFT_RED);
}

static void redraw_snake(void)
{
    uint8_t index;
    draw_header("SNAKE");
    tft_draw_text(4U, 11U, "JOYSTICK: RUCH  SW: PAUZA", TFT_WHITE,
                  TFT_BLACK, 1U);
    for (index = 0U; index < snake_length; ++index)
    {
        tft_fill_rect(PLAY_LEFT + snake[index].x * SNAKE_CELL,
                      PLAY_TOP + snake[index].y * SNAKE_CELL,
                      SNAKE_CELL - 1U, SNAKE_CELL - 1U, TFT_GREEN);
    }
    tft_fill_rect(PLAY_LEFT + food.x * SNAKE_CELL,
                  PLAY_TOP + food.y * SNAKE_CELL, SNAKE_CELL - 1U,
                  SNAKE_CELL - 1U, TFT_RED);
}

static void redraw_dodge(void)
{
    uint8_t index;
    draw_header("KROPKA");
    tft_draw_text(4U, 11U, "UNIKAJ BOMB  SW: PAUZA", TFT_WHITE,
                  TFT_BLACK, 1U);
    for (index = 0U; index < bomb_count; ++index)
    {
        tft_fill_rect(bombs[index].x - 2U, bombs[index].y - 2U, 5U, 5U,
                      TFT_RED);
    }
    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
}

static void start_dodge(uint32_t seed)
{
    screen = GAME_DODGE;
    random_state = seed ^ 0x3D71B2C9UL;
    frame = 0U;
    score = 0U;
    bomb_count = 0U;
    player_x = TFT_WIDTH / 2U;
    player_y = 82U;
    draw_header("KROPKA  CZAS: 0");
    tft_draw_text(4U, 11U, "UNIKAJ BOMB  SW: PAUZA", TFT_WHITE,
                  TFT_BLACK, 1U);
    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
}

static void open_pause(void)
{
    paused_screen = screen;
    screen = GAME_PAUSE;
    pause_item = 0U;
    draw_pause();
}

static uint8_t direction_is_up(JoystickDirection direction)
{
    return direction == JOY_UP || direction == JOY_UP_LEFT ||
           direction == JOY_UP_RIGHT;
}

static uint8_t direction_is_down(JoystickDirection direction)
{
    return direction == JOY_DOWN || direction == JOY_DOWN_LEFT ||
           direction == JOY_DOWN_RIGHT;
}

static void update_snake(JoystickState state)
{
    Position head;
    uint8_t index;

    if (state.direction == JOY_UP && snake_direction != JOY_DOWN)
        snake_direction = JOY_UP;
    else if (state.direction == JOY_DOWN && snake_direction != JOY_UP)
        snake_direction = JOY_DOWN;
    else if (state.direction == JOY_LEFT && snake_direction != JOY_RIGHT)
        snake_direction = JOY_LEFT;
    else if (state.direction == JOY_RIGHT && snake_direction != JOY_LEFT)
        snake_direction = JOY_RIGHT;
    if ((frame++ & 3U) != 0U) return;

    head = snake[0];
    if (snake_direction == JOY_UP) --head.y;
    if (snake_direction == JOY_DOWN) ++head.y;
    if (snake_direction == JOY_LEFT) --head.x;
    if (snake_direction == JOY_RIGHT) ++head.x;
    if (head.x >= SNAKE_COLS || head.y >= SNAKE_ROWS)
    {
        screen = GAME_OVER;
        draw_game_over();
        return;
    }
    for (index = 0U; index < snake_length; ++index)
    {
        if (snake[index].x == head.x && snake[index].y == head.y)
        {
            screen = GAME_OVER;
            draw_game_over();
            return;
        }
    }
    if (head.x == food.x && head.y == food.y)
    {
        if (snake_length < MAX_SNAKE) ++snake_length;
        ++score;
        food.x = (uint16_t)(next_random() % SNAKE_COLS);
        food.y = (uint16_t)(next_random() % SNAKE_ROWS);
    }
    else
    {
        tft_fill_rect(PLAY_LEFT + snake[snake_length - 1U].x * SNAKE_CELL,
                      PLAY_TOP + snake[snake_length - 1U].y * SNAKE_CELL,
                      SNAKE_CELL - 1U, SNAKE_CELL - 1U, TFT_BLACK);
    }
    for (index = snake_length - 1U; index > 0U; --index) snake[index] = snake[index - 1U];
    snake[0] = head;
    tft_fill_rect(PLAY_LEFT + head.x * SNAKE_CELL,
                  PLAY_TOP + head.y * SNAKE_CELL, SNAKE_CELL - 1U,
                  SNAKE_CELL - 1U, TFT_GREEN);
    tft_fill_rect(PLAY_LEFT + food.x * SNAKE_CELL,
                  PLAY_TOP + food.y * SNAKE_CELL, SNAKE_CELL - 1U,
                  SNAKE_CELL - 1U, TFT_RED);
}

static void update_dodge(JoystickState state)
{
    uint8_t index;
    if (direction_is_up(state.direction) && player_y > PLAY_TOP + 4U) player_y -= 2U;
    if (direction_is_down(state.direction) && player_y < PLAY_BOTTOM - 4U) player_y += 2U;
    if ((state.direction == JOY_LEFT || state.direction == JOY_UP_LEFT ||
         state.direction == JOY_DOWN_LEFT) && player_x > PLAY_LEFT + 4U) player_x -= 2U;
    if ((state.direction == JOY_RIGHT || state.direction == JOY_UP_RIGHT ||
         state.direction == JOY_DOWN_RIGHT) && player_x < PLAY_RIGHT - 4U) player_x += 2U;
    ++frame;
    tft_fill_rect(PLAY_LEFT, PLAY_TOP, PLAY_RIGHT - PLAY_LEFT + 1U,
                  PLAY_BOTTOM - PLAY_TOP + 1U, TFT_BLACK);
    for (index = 0U; index < bomb_count; ++index)
    {
        if (frame % 2U == 0U) ++bombs[index].y;
        if (bombs[index].y > PLAY_BOTTOM) bombs[index].y = PLAY_TOP;
        tft_fill_rect(bombs[index].x - 2U, bombs[index].y - 2U, 5U, 5U, TFT_RED);
        if (player_x + 2U >= bombs[index].x - 2U &&
            player_x - 2U <= bombs[index].x + 2U &&
            player_y + 2U >= bombs[index].y - 2U &&
            player_y - 2U <= bombs[index].y + 2U)
        {
            score = frame / 33U;
            screen = GAME_OVER;
            draw_game_over();
            return;
        }
    }
    if (bomb_count < MAX_BOMBS && frame >= 15U &&
        frame % (45U > score / 3U ? 45U - score / 3U : 12U) == 0U)
    {
        bombs[bomb_count].x = (uint16_t)(PLAY_LEFT + 3U + next_random() %
                                         (PLAY_RIGHT - PLAY_LEFT - 5U));
        bombs[bomb_count].y = PLAY_TOP + 2U;
        ++bomb_count;
    }
    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
    if ((frame % 33U) == 0U)
    {
        score = frame / 33U;
        tft_draw_uint(70U, 2U, score, TFT_YELLOW, TFT_BLACK, 1U);
    }
}

void game_init(void)
{
    screen = GAME_MENU;
    menu_item = 0U;
    last_menu_direction = JOY_CENTER;
    draw_menu();
}

void game_update(JoystickState state, uint32_t button_event)
{
    if (screen == GAME_MENU)
    {
        if (state.direction != last_menu_direction)
        {
            if (direction_is_up(state.direction) || direction_is_down(state.direction))
            {
                menu_item = (uint8_t)(1U - menu_item);
                draw_menu();
            }
            last_menu_direction = state.direction;
        }
        if (button_event != 0U)
        {
            if (menu_item == 0U) start_snake(state.x ^ ((uint32_t)state.y << 16));
            else start_dodge(state.x ^ ((uint32_t)state.y << 16));
        }
        return;
    }
    if (screen == GAME_PAUSE)
    {
        if (state.direction != last_menu_direction)
        {
            if (direction_is_up(state.direction) || direction_is_down(state.direction))
            {
                pause_item = (uint8_t)(1U - pause_item);
                draw_pause();
            }
            last_menu_direction = state.direction;
        }
        if (button_event != 0U)
        {
            if (pause_item == 0U)
            {
                screen = paused_screen;
                if (screen == GAME_SNAKE) redraw_snake();
                else redraw_dodge();
            }
            else
            {
                game_init();
            }
        }
        return;
    }
    if (screen == GAME_OVER)
    {
        if (button_event != 0U) game_init();
        return;
    }
    if (button_event != 0U)
    {
        open_pause();
        return;
    }
    if (screen == GAME_SNAKE) update_snake(state);
    else if (screen == GAME_DODGE) update_dodge(state);
}
