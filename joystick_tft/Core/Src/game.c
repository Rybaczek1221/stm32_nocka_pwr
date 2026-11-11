#include "game.h"
#include "tft_st7735.h"
#include "lives.h"

#define PLAY_TOP 28U
#define PLAY_LEFT 8U
#define PLAY_RIGHT 151U
#define PLAY_BOTTOM 123U
#define SNAKE_CELL 8U
#define SNAKE_COLS 18U
#define SNAKE_ROWS 11U
#define MAX_SNAKE 60U
#define MAX_BOMBS 10U
#define AIM_START_TENTHS 50U
#define AIM_MAX_OBSTACLES 5U

typedef struct
{
    uint16_t x;
    uint16_t y;
} Position;

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} AimObstacle;

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
static Position snake_bonus;
static uint8_t snake_bonus_active;
static JoystickDirection snake_direction;
static Position bombs[MAX_BOMBS];
static uint8_t bomb_count;
static uint16_t player_x;
static uint16_t player_y;
static uint16_t target_x;
static uint16_t target_y;
static uint16_t target_radius;
static uint16_t aim_round_frames;
static uint8_t aim_time_tenths;
static uint8_t aim_obstacle_count;
static AimObstacle aim_obstacles[AIM_MAX_OBSTACLES];

static uint8_t direction_is_up(JoystickDirection direction);
static uint8_t direction_is_down(JoystickDirection direction);
static void draw_game_over(void);
static void redraw_aim(void);
static void draw_aim_obstacles(void);
static void start_snake(uint32_t seed);
static void start_dodge(uint32_t seed);
static void start_aim(uint32_t seed);

static uint8_t lose_life_or_end(void)
{
    if (lives_lose() != 0U) return 1U;
    screen = GAME_OVER;
    draw_game_over();
    return 0U;
}

static uint32_t next_random(void)
{
    /* A small deterministic generator is enough for repeatable arcade logic. */
    random_state = random_state * 1664525UL + 1013904223UL;
    return random_state;
}

static void draw_target(void)
{
    uint16_t diameter = (uint16_t)(target_radius * 2U + 1U);
    uint16_t middle_height = (uint16_t)(diameter - 6U);
    tft_fill_rect(target_x - target_radius + 3U, target_y - target_radius,
                  (uint16_t)(diameter - 6U), 3U, TFT_CYAN);
    tft_fill_rect(target_x - target_radius, target_y - target_radius + 3U,
                  diameter, middle_height, TFT_CYAN);
    tft_fill_rect(target_x - target_radius + 3U, target_y + target_radius - 2U,
                  (uint16_t)(diameter - 6U), 3U, TFT_CYAN);
    tft_fill_rect(target_x - target_radius + 5U, target_y - target_radius + 3U,
                  (uint16_t)(diameter - 10U), 3U, TFT_NAVY);
    tft_fill_rect(target_x - target_radius + 3U, target_y - 2U,
                  (uint16_t)(diameter - 6U), 5U, TFT_NAVY);
    tft_fill_rect(target_x - target_radius + 5U, target_y + target_radius - 1U,
                  (uint16_t)(diameter - 10U), 3U, TFT_NAVY);
}

static void draw_aim_obstacles(void)
{
    uint8_t index;
    for (index = 0U; index < aim_obstacle_count; ++index)
    {
        tft_fill_rect(aim_obstacles[index].x, aim_obstacles[index].y,
                      aim_obstacles[index].width, aim_obstacles[index].height,
                      TFT_RED);
        tft_fill_rect(aim_obstacles[index].x + 2U,
                      aim_obstacles[index].y + 2U,
                      aim_obstacles[index].width - 4U,
                      aim_obstacles[index].height - 4U, TFT_PURPLE);
    }
}

static void draw_aim_status(uint16_t remaining_tenths)
{
    tft_fill_rect(103U, 1U, 53U, 17U, TFT_PURPLE);
    tft_draw_text(104U, 5U, "CZAS", TFT_CYAN, TFT_PURPLE, 1U);
    tft_draw_uint(134U, 5U, remaining_tenths / 10U, TFT_YELLOW,
                  TFT_PURPLE, 1U);
    tft_draw_text(140U, 5U, ".", TFT_WHITE, TFT_PURPLE, 1U);
    tft_draw_uint(146U, 5U, remaining_tenths % 10U, TFT_YELLOW,
                  TFT_PURPLE, 1U);
}

static uint8_t player_hits_obstacle(void)
{
    uint8_t index;
    for (index = 0U; index < aim_obstacle_count; ++index)
    {
        if (player_x + 2U >= aim_obstacles[index].x &&
            player_x <= aim_obstacles[index].x + aim_obstacles[index].width &&
            player_y + 2U >= aim_obstacles[index].y &&
            player_y <= aim_obstacles[index].y + aim_obstacles[index].height)
        {
            return 1U;
        }
    }
    return 0U;
}

static void create_aim_obstacles(void)
{
    uint8_t index;
    aim_obstacle_count = (uint8_t)(1U + score / 2U);
    if (aim_obstacle_count > AIM_MAX_OBSTACLES) aim_obstacle_count = AIM_MAX_OBSTACLES;
    for (index = 0U; index < aim_obstacle_count; ++index)
    {
        aim_obstacles[index].width = (uint16_t)(10U + next_random() % 13U);
        aim_obstacles[index].height = (uint16_t)(8U + next_random() % 12U);
        aim_obstacles[index].x = (uint16_t)(PLAY_LEFT + 4U +
                                             next_random() % 115U);
        aim_obstacles[index].y = (uint16_t)(PLAY_TOP + 8U +
                                             next_random() % 75U);
    }
}

static void start_aim_round(void)
{
    target_x = (uint16_t)(20U + next_random() % 120U);
    target_y = (uint16_t)(38U + next_random() % 70U);
    aim_time_tenths = AIM_START_TENTHS > score ?
                      (uint8_t)(AIM_START_TENTHS - score) : 1U;
    aim_round_frames = 0U;
    create_aim_obstacles();
}

static void draw_header(const char *title)
{
    tft_fill_screen(TFT_NAVY);
    tft_fill_rect(0U, 0U, TFT_WIDTH, 19U, TFT_PURPLE);
    tft_draw_text(4U, 5U, title, TFT_YELLOW, TFT_PURPLE, 1U);
}

static void draw_snake_cell(Position cell, uint16_t color)
{
    tft_fill_rect(PLAY_LEFT + cell.x * SNAKE_CELL + 1U,
                  PLAY_TOP + cell.y * SNAKE_CELL + 1U,
                  SNAKE_CELL - 2U, SNAKE_CELL - 2U, color);
}

static void draw_snake_board(void)
{
    uint8_t column;
    uint8_t row;
    tft_fill_rect(PLAY_LEFT - 2U, PLAY_TOP - 2U,
                  SNAKE_COLS * SNAKE_CELL + 4U,
                  SNAKE_ROWS * SNAKE_CELL + 4U, TFT_VIOLET);
    tft_fill_rect(PLAY_LEFT, PLAY_TOP, SNAKE_COLS * SNAKE_CELL,
                  SNAKE_ROWS * SNAKE_CELL, TFT_PANEL);
    for (column = 0U; column <= SNAKE_COLS; ++column)
    {
        tft_fill_rect(PLAY_LEFT + column * SNAKE_CELL, PLAY_TOP, 1U,
                      SNAKE_ROWS * SNAKE_CELL, TFT_GRID);
    }
    for (row = 0U; row <= SNAKE_ROWS; ++row)
    {
        tft_fill_rect(PLAY_LEFT, PLAY_TOP + row * SNAKE_CELL,
                      SNAKE_COLS * SNAKE_CELL, 1U, TFT_GRID);
    }
}

static void draw_snake_status(void)
{
    tft_fill_rect(38U, 1U, 118U, 17U, TFT_PURPLE);
    tft_draw_text(41U, 5U, "WYNIK", TFT_WHITE, TFT_PURPLE, 1U);
    tft_draw_uint(75U, 5U, score, TFT_YELLOW, TFT_PURPLE, 1U);
    tft_draw_text(100U, 5U, "CZAS", TFT_CYAN, TFT_PURPLE, 1U);
    tft_draw_uint(130U, 5U, frame / 33U, TFT_WHITE, TFT_PURPLE, 1U);
}

static void update_aim(JoystickState state)
{
    uint16_t old_x = player_x;
    uint16_t old_y = player_y;
    uint16_t distance_x;
    uint16_t distance_y;
    ++frame;
    if (direction_is_up(state.direction) && player_y > PLAY_TOP + 4U) player_y -= 2U;
    if (direction_is_down(state.direction) && player_y < PLAY_BOTTOM - 4U) player_y += 2U;
    if ((state.direction == JOY_LEFT || state.direction == JOY_UP_LEFT ||
         state.direction == JOY_DOWN_LEFT) && player_x > PLAY_LEFT + 4U) player_x -= 2U;
    if ((state.direction == JOY_RIGHT || state.direction == JOY_UP_RIGHT ||
         state.direction == JOY_DOWN_RIGHT) && player_x < PLAY_RIGHT - 4U) player_x += 2U;
    if (old_x != player_x || old_y != player_y)
    {
        tft_fill_rect(old_x - 3U, old_y - 3U, 7U, 7U, TFT_BLACK);
        draw_target();
        draw_aim_obstacles();
        tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
    }
    if (player_hits_obstacle())
    {
        screen = GAME_OVER;
        draw_game_over();
        return;
    }
    ++aim_round_frames;
    if ((aim_round_frames % 33U) == 0U)
    {
        uint16_t total_frames = (uint16_t)((aim_time_tenths * 33U + 5U) / 10U);
        uint16_t remaining_tenths = aim_round_frames >= total_frames ? 0U :
                                    (uint16_t)(((total_frames - aim_round_frames) * 10U + 32U) / 33U);
        draw_aim_status(remaining_tenths);
    }
    /* One main-loop tick is about 30 ms, so tenths are calculated from ticks. */
    if (aim_round_frames < (uint16_t)((aim_time_tenths * 33U + 5U) / 10U)) return;
    distance_x = player_x > target_x ? player_x - target_x : target_x - player_x;
    distance_y = player_y > target_y ? player_y - target_y : target_y - player_y;
    if (distance_x * distance_x + distance_y * distance_y >
        target_radius * target_radius)
    {
        if (lose_life_or_end() != 0U) start_aim(next_random());
        return;
    }
    ++score;
    if (target_radius > 5U) --target_radius;
    start_aim_round();
    redraw_aim();
}

static void draw_menu(void)
{
    tft_fill_screen(TFT_NAVY);
    tft_fill_rect(4U, 3U, 152U, 23U, TFT_PURPLE);
    tft_fill_rect(7U, 6U, 146U, 17U, TFT_NAVY);
    tft_draw_text(18U, 9U, "JAROSLAW RYBAK", TFT_YELLOW, TFT_NAVY, 1U);
    tft_draw_text(68U, 31U, "PWR", TFT_CYAN, TFT_NAVY, 1U);
    tft_draw_text(20U, 43U, "WYBIERZ GRE", TFT_WHITE, TFT_NAVY, 1U);
    tft_fill_rect(12U, 54U, 136U, 15U, menu_item == 0U ? TFT_VIOLET : TFT_PANEL);
    tft_fill_rect(12U, 71U, 136U, 15U, menu_item == 1U ? TFT_VIOLET : TFT_PANEL);
    tft_fill_rect(12U, 88U, 136U, 15U, menu_item == 2U ? TFT_VIOLET : TFT_PANEL);
    tft_draw_text(24U, 57U, "SNAKE", menu_item == 0U ? TFT_YELLOW : TFT_WHITE,
                  menu_item == 0U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(24U, 74U, "KROPKA", menu_item == 1U ? TFT_YELLOW : TFT_WHITE,
                  menu_item == 1U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(24U, 91U, "CELUJ", menu_item == 2U ? TFT_YELLOW : TFT_WHITE,
                  menu_item == 2U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(18U, 113U, "SW START  GORA DOL", TFT_GREEN, TFT_NAVY, 1U);
}

static void draw_pause(void)
{
    tft_fill_rect(12U, 31U, 136U, 66U, TFT_PURPLE);
    tft_fill_rect(15U, 34U, 130U, 60U, TFT_PANEL);
    tft_draw_text(50U, 40U, "PAUZA", TFT_YELLOW, TFT_PANEL, 1U);
    tft_draw_text(28U, 58U, pause_item == 0U ? "> WZNOW" : "  WZNOW",
                  pause_item == 0U ? TFT_WHITE : TFT_CYAN, TFT_PANEL, 1U);
    tft_draw_text(28U, 75U, pause_item == 1U ? "> MENU" : "  MENU",
                  pause_item == 1U ? TFT_WHITE : TFT_CYAN, TFT_PANEL, 1U);
}

static void draw_game_over(void)
{
    tft_fill_screen(TFT_NAVY);
    tft_fill_rect(10U, 18U, 140U, 84U, TFT_PURPLE);
    tft_fill_rect(13U, 21U, 134U, 78U, TFT_PANEL);
    tft_draw_text(29U, 32U, "KONIEC GRY", TFT_YELLOW, TFT_PANEL, 1U);
    tft_draw_text(31U, 52U, "WYNIK", TFT_WHITE, TFT_PANEL, 1U);
    tft_draw_uint(75U, 52U, score, TFT_CYAN, TFT_PANEL, 1U);
    tft_draw_text(30U, 76U, "CZAS", TFT_WHITE, TFT_PANEL, 1U);
    tft_draw_uint(65U, 76U, frame / 33U, TFT_CYAN, TFT_PANEL, 1U);
    tft_draw_text(22U, 91U, "SW MENU", TFT_GREEN, TFT_PANEL, 1U);
}

static void start_snake(uint32_t seed)
{
    uint8_t index;
    screen = GAME_SNAKE;
    random_state = seed ^ 0xA53C9E17UL;
    frame = 0U;
    score = 0U;
    snake_length = 4U;
    snake_bonus_active = 0U;
    snake_direction = JOY_RIGHT;
    for (index = 0U; index < snake_length; ++index)
    {
        snake[index].x = 10U - index;
        snake[index].y = 8U;
    }
    food.x = (uint16_t)(next_random() % SNAKE_COLS);
    food.y = (uint16_t)(next_random() % SNAKE_ROWS);
    draw_header("SNAKE");
    draw_snake_board();
    draw_snake_status();
    for (index = 0U; index < snake_length; ++index)
    {
        draw_snake_cell(snake[index], index == 0U ? TFT_CYAN : TFT_VIOLET);
    }
    draw_snake_cell(food, TFT_ORANGE);
}

static void redraw_snake(void)
{
    uint8_t index;
    draw_header("SNAKE");
    draw_snake_board();
    draw_snake_status();
    for (index = 0U; index < snake_length; ++index)
    {
        draw_snake_cell(snake[index], index == 0U ? TFT_CYAN : TFT_VIOLET);
    }
    draw_snake_cell(food, TFT_ORANGE);
    if (snake_bonus_active) draw_snake_cell(snake_bonus, TFT_YELLOW);
}

static void redraw_dodge(void)
{
    uint8_t index;
    draw_header("KROPKA");
    tft_draw_text(4U, 21U, "UNIKAJ BOMB  SW PAUZA", TFT_WHITE,
                  TFT_NAVY, 1U);
    for (index = 0U; index < bomb_count; ++index)
    {
        tft_fill_rect(bombs[index].x - 2U, bombs[index].y - 2U, 5U, 5U,
                      TFT_RED);
    }
    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
}

static void start_aim(uint32_t seed)
{
    screen = GAME_AIM;
    random_state = seed ^ 0x7B19D421UL;
    frame = 0U;
    score = 0U;
    player_x = TFT_WIDTH / 2U;
    player_y = 82U;
    target_radius = 12U;
    start_aim_round();
    draw_header("CELUJ");
    tft_draw_text(4U, 21U, "TRAF W STREFE  SW PAUZA", TFT_WHITE,
                  TFT_NAVY, 1U);
    draw_aim_status(aim_time_tenths);
    draw_target();
    draw_aim_obstacles();
    tft_fill_rect(player_x - 2U, player_y - 2U, 5U, 5U, TFT_YELLOW);
}

static void redraw_aim(void)
{
    uint16_t total_frames = (uint16_t)((aim_time_tenths * 33U + 5U) / 10U);
    uint16_t remaining_tenths = aim_round_frames >= total_frames ? 0U :
                                (uint16_t)(((total_frames - aim_round_frames) * 10U + 32U) / 33U);
    draw_header("CELUJ");
    tft_draw_text(4U, 21U, "TRAF W STREFE  SW PAUZA", TFT_WHITE,
                  TFT_NAVY, 1U);
    draw_aim_status(remaining_tenths);
    draw_target();
    draw_aim_obstacles();
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
    draw_header("KROPKA");
    tft_draw_text(4U, 21U, "UNIKAJ BOMB  SW PAUZA", TFT_WHITE,
                  TFT_NAVY, 1U);
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
    Position tail;
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
        if (lose_life_or_end() != 0U) start_snake(next_random());
        return;
    }
    for (index = 0U; index < snake_length; ++index)
    {
        if (snake[index].x == head.x && snake[index].y == head.y)
        {
            if (lose_life_or_end() != 0U) start_snake(next_random());
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
        tail = snake[snake_length - 1U];
        draw_snake_cell(tail, TFT_PANEL);
    }
    if (snake_bonus_active &&
        head.x == snake_bonus.x && head.y == snake_bonus.y)
    {
        score += 5U;
        snake_bonus_active = 0U;
    }
    for (index = snake_length - 1U; index > 0U; --index) snake[index] = snake[index - 1U];
    snake[0] = head;
    draw_snake_cell(head, TFT_CYAN);
    if (snake_length > 1U)
    {
        draw_snake_cell(snake[1], TFT_VIOLET);
    }
    draw_snake_cell(food, TFT_ORANGE);
    if (!snake_bonus_active && frame > 0U && (frame % 330U) == 0U)
    {
        snake_bonus.x = (uint16_t)(next_random() % SNAKE_COLS);
        snake_bonus.y = (uint16_t)(next_random() % SNAKE_ROWS);
        snake_bonus_active = 1U;
    }
    if (snake_bonus_active) draw_snake_cell(snake_bonus, TFT_YELLOW);
    if ((frame % 33U) == 0U) draw_snake_status();
}

static void update_dodge(JoystickState state)
{
    uint8_t index;
    uint16_t old_x = player_x;
    uint16_t old_y = player_y;
    for (index = 0U; index < bomb_count; ++index)
    {
        tft_fill_rect(bombs[index].x - 2U, bombs[index].y - 2U, 5U, 5U,
                      TFT_BLACK);
    }
    tft_fill_rect(old_x - 2U, old_y - 2U, 5U, 5U, TFT_BLACK);
    if (direction_is_up(state.direction) && player_y > PLAY_TOP + 4U) player_y -= 2U;
    if (direction_is_down(state.direction) && player_y < PLAY_BOTTOM - 4U) player_y += 2U;
    if ((state.direction == JOY_LEFT || state.direction == JOY_UP_LEFT ||
         state.direction == JOY_DOWN_LEFT) && player_x > PLAY_LEFT + 4U) player_x -= 2U;
    if ((state.direction == JOY_RIGHT || state.direction == JOY_UP_RIGHT ||
         state.direction == JOY_DOWN_RIGHT) && player_x < PLAY_RIGHT - 4U) player_x += 2U;
    ++frame;
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
            if (lose_life_or_end() != 0U) start_dodge(next_random());
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
    lives_reset();
    draw_menu();
}

void game_update(JoystickState state, uint32_t button_event)
{
    /* All screen transitions stay here so input handling is consistent. */
    if (screen == GAME_MENU)
    {
        if (state.direction != last_menu_direction)
        {
            if (direction_is_up(state.direction) || direction_is_down(state.direction))
            {
                menu_item = (uint8_t)((menu_item + (direction_is_down(state.direction) ? 1U : 2U)) % 3U);
                draw_menu();
            }
            last_menu_direction = state.direction;
        }
        if (button_event != 0U)
        {
            if (menu_item == 0U) start_snake(state.x ^ ((uint32_t)state.y << 16));
            else if (menu_item == 1U) start_dodge(state.x ^ ((uint32_t)state.y << 16));
            else start_aim(state.x ^ ((uint32_t)state.y << 16));
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
                else if (screen == GAME_DODGE) redraw_dodge();
                else redraw_aim();
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
    else if (screen == GAME_AIM) update_aim(state);
}
