#include "game.h"
#include "tft_st7735.h"
#include "lives.h"
#include "mpu6050.h"

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
#define AIM_LEVELS 10U
#define FLIGHT_OBSTACLES 4U
#define FLIGHT_CALIBRATION_SAMPLES 12U
#define SENSOR_TILT_DEADZONE 900
#define SENSOR_TILT_SCALE 9000
#define BALANCE_TARGET_RADIUS 7U

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

typedef struct
{
    uint16_t x;
    uint16_t y;
    int8_t dx;
    int8_t dy;
} Bomb;

typedef struct
{
    uint16_t x;
    uint16_t gap_y;
    uint16_t gap_height;
} FlightObstacle;

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
static Bomb bombs[MAX_BOMBS];
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
static uint8_t aim_level;
static Mpu6050Data telemetry_data;
static uint8_t telemetry_ok;
static uint8_t telemetry_frame;
static uint8_t telemetry_identity;
static uint32_t telemetry_samples;
static FlightObstacle flight_obstacles[FLIGHT_OBSTACLES];
static uint16_t paint_x;
static uint16_t paint_y;
static uint8_t paint_color;
static uint32_t paint_sensor_cooldown;
static uint8_t led_test_item;
static uint8_t led_test_state[5];
static uint8_t sensor_menu_item;
static uint8_t flight_calibrating;
static uint8_t flight_calibration_samples;
static uint8_t flight_calibration_valid;
static int32_t flight_calibration_sum_y;
static int32_t flight_calibration_sum_x;
static int16_t flight_neutral_y;
static int16_t flight_neutral_x;
static int32_t sensor_filtered_x;
static int32_t sensor_filtered_y;
static uint16_t balance_x;
static uint16_t balance_y;
static uint16_t balance_target_x;
static uint16_t balance_target_y;
static uint16_t balance_time;
static uint16_t balance_score;

static uint8_t direction_is_up(JoystickDirection direction);
static uint8_t direction_is_down(JoystickDirection direction);
static void draw_game_over(void);
static void redraw_aim(void);
static void draw_aim_obstacles(void);
static void start_snake(uint32_t seed);
static void start_dodge(uint32_t seed);
static void start_aim(uint32_t seed);
static void start_aim_round(void);
static void draw_telemetry(void);
static void start_flight(uint32_t seed);
static void update_flight(JoystickState state);
static void redraw_flight(void);
static void start_paint(void);
static void update_paint(JoystickState state);
static void draw_paint(void);
static void start_balance(void);
static void update_balance(void);
static void draw_balance(void);
static void draw_sensor_menu(void);
static void draw_led_info(void);

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

static uint8_t player_reached_target(void)
{
    uint16_t distance_x = player_x > target_x ? player_x - target_x :
                          target_x - player_x;
    uint16_t distance_y = player_y > target_y ? player_y - target_y :
                          target_y - player_y;
    uint16_t reach_radius = (uint16_t)(target_radius + 3U);
    return distance_x * distance_x + distance_y * distance_y <=
           reach_radius * reach_radius;
}

static void complete_aim_level(void)
{
    ++score;
    if (aim_level >= AIM_LEVELS)
    {
        frame = aim_round_frames;
        screen = GAME_OVER;
        draw_game_over();
        return;
    }
    ++aim_level;
    if (target_radius > 4U) --target_radius;
    start_aim_round();
    redraw_aim();
}

static void create_aim_obstacles(void)
{
    uint8_t index;
    aim_obstacle_count = (uint8_t)(2U + aim_level);
    if (aim_obstacle_count > AIM_MAX_OBSTACLES) aim_obstacle_count = AIM_MAX_OBSTACLES;
    for (index = 0U; index < aim_obstacle_count; ++index)
    {
        aim_obstacles[index].width = (uint16_t)(8U + next_random() % 18U);
        aim_obstacles[index].height = (uint16_t)(6U + next_random() % 14U);
        aim_obstacles[index].x = (uint16_t)(PLAY_LEFT + 8U +
                                             next_random() % 105U);
        aim_obstacles[index].y = (uint16_t)(PLAY_TOP + 8U +
                                             next_random() % 68U);
    }
}

static void start_aim_round(void)
{
    player_x = PLAY_LEFT + 8U;
    player_y = PLAY_BOTTOM - 8U;
    target_x = PLAY_RIGHT - 8U;
    target_y = PLAY_TOP + 10U;
    aim_time_tenths = (uint8_t)(120U - aim_level * 5U);
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
    if (player_reached_target())
    {
        complete_aim_level();
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
    if (!player_reached_target())
    {
        if (lose_life_or_end() != 0U) start_aim(next_random());
        return;
    }
    complete_aim_level();
}

static void draw_menu(void)
{
    tft_fill_screen(TFT_NAVY);
    tft_fill_rect(4U, 3U, 152U, 23U, TFT_PURPLE);
    tft_fill_rect(7U, 6U, 146U, 17U, TFT_NAVY);
    tft_draw_text(8U, 9U, "JAROSLAW RYBAK PWR", TFT_YELLOW, TFT_NAVY, 1U);
    tft_draw_text(20U, 34U, "WYBIERZ TRYB", TFT_WHITE, TFT_NAVY, 1U);
    tft_fill_rect(12U, 44U, 136U, 14U, menu_item == 0U ? TFT_VIOLET : TFT_PANEL);
    tft_fill_rect(12U, 61U, 136U, 14U, menu_item == 1U ? TFT_VIOLET : TFT_PANEL);
    tft_fill_rect(12U, 78U, 136U, 14U, menu_item == 2U ? TFT_VIOLET : TFT_PANEL);
    tft_fill_rect(12U, 95U, 136U, 14U, menu_item == 3U ? TFT_VIOLET : TFT_PANEL);
    tft_draw_text(24U, 47U, "SNAKE", menu_item == 0U ? TFT_YELLOW : TFT_WHITE,
                  menu_item == 0U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(24U, 64U, "KROPKA", menu_item == 1U ? TFT_YELLOW : TFT_WHITE,
                  menu_item == 1U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(24U, 81U, "LABIRYNT", menu_item == 2U ? TFT_YELLOW : TFT_WHITE,
                  menu_item == 2U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(24U, 98U, "CZUJNIKI", menu_item == 3U ? TFT_YELLOW : TFT_WHITE,
                  menu_item == 3U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(28U, 115U, "GORA DOL  SW START", TFT_GREEN, TFT_NAVY, 1U);
}

static void draw_signed(uint16_t x, uint16_t y, int16_t value)
{
    uint16_t magnitude = value < 0 ? (uint16_t)(-value) : (uint16_t)value;
    tft_draw_text(x, y, value < 0 ? "-" : "+", TFT_WHITE, TFT_NAVY, 1U);
    tft_draw_uint(x + 6U, y, magnitude, TFT_CYAN, TFT_NAVY, 1U);
}

static void draw_sensor_menu(void)
{
    draw_header("CZUJNIKI");
    tft_draw_text(18U, 23U, "WYBIERZ TRYB", TFT_WHITE, TFT_NAVY, 1U);
    tft_fill_rect(12U, 38U, 136U, 15U,
                  sensor_menu_item == 0U ? TFT_VIOLET : TFT_PANEL);
    tft_fill_rect(12U, 56U, 136U, 15U,
                  sensor_menu_item == 1U ? TFT_VIOLET : TFT_PANEL);
    tft_fill_rect(12U, 74U, 136U, 15U,
                  sensor_menu_item == 2U ? TFT_VIOLET : TFT_PANEL);
    tft_fill_rect(12U, 92U, 136U, 15U,
                  sensor_menu_item == 3U ? TFT_VIOLET : TFT_PANEL);
    tft_draw_text(28U, 41U, "TELEMETRIA",
                  sensor_menu_item == 0U ? TFT_YELLOW : TFT_WHITE,
                  sensor_menu_item == 0U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(28U, 59U, "PAINT",
                  sensor_menu_item == 1U ? TFT_YELLOW : TFT_WHITE,
                  sensor_menu_item == 1U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(28U, 77U, "BALANS",
                  sensor_menu_item == 2U ? TFT_YELLOW : TFT_WHITE,
                  sensor_menu_item == 2U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(28U, 95U, "LEDY ZYCIA",
                  sensor_menu_item == 3U ? TFT_YELLOW : TFT_WHITE,
                  sensor_menu_item == 3U ? TFT_VIOLET : TFT_PANEL, 1U);
    tft_draw_text(13U, 112U, "GORA DOL  SW WYBIERZ", TFT_GREEN, TFT_NAVY, 1U);
}

static void draw_led_info(void)
{
    draw_header("LEDY ZYCIA");
    static const char *names[5] = {
        "PC6 CZERWONA", "PC7 CZERWONA", "PC8 ZOLTA",
        "PC9 ZIELONA", "PC10 ZIELONA"
    };
    static const uint16_t colors[5] = {
        TFT_RED, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_GREEN
    };
    uint8_t index;
    for (index = 0U; index < 5U; ++index)
    {
        tft_fill_rect(12U, 27U + index * 17U, 136U, 14U,
                      index == led_test_item ? TFT_VIOLET : TFT_PANEL);
        tft_fill_rect(18U, 30U + index * 17U, 8U, 8U,
                      led_test_state[index] != 0U ? colors[index] : TFT_BLACK);
        tft_draw_text(32U, 30U + index * 17U, names[index],
                      index == led_test_item ? TFT_YELLOW : TFT_WHITE,
                      index == led_test_item ? TFT_VIOLET : TFT_PANEL, 1U);
    }
    tft_draw_text(18U, 114U, "GORA DOL  L/P ZMIEN", TFT_GREEN, TFT_NAVY, 1U);
}

static void draw_telemetry(void)
{
    draw_header("CZUJNIKI");
    tft_draw_text(4U, 22U, telemetry_ok ? "I2C TEST OK" : "I2C TEST ERROR",
                  telemetry_ok ? TFT_GREEN : TFT_RED, TFT_NAVY, 1U);
    tft_draw_text(91U, 22U, "WHO", TFT_CYAN, TFT_NAVY, 1U);
    tft_draw_uint(115U, 22U, telemetry_identity, TFT_YELLOW, TFT_NAVY, 1U);
    tft_draw_text(8U, 39U, "ACC X", TFT_WHITE, TFT_NAVY, 1U);
    draw_signed(48U, 39U, telemetry_data.accel_x);
    tft_draw_text(8U, 55U, "ACC Y", TFT_WHITE, TFT_NAVY, 1U);
    draw_signed(48U, 55U, telemetry_data.accel_y);
    tft_draw_text(8U, 71U, "ACC Z", TFT_WHITE, TFT_NAVY, 1U);
    draw_signed(48U, 71U, telemetry_data.accel_z);
    tft_draw_text(88U, 39U, "GYR X", TFT_WHITE, TFT_NAVY, 1U);
    draw_signed(128U, 39U, telemetry_data.gyro_x);
    tft_draw_text(88U, 55U, "GYR Y", TFT_WHITE, TFT_NAVY, 1U);
    draw_signed(128U, 55U, telemetry_data.gyro_y);
    tft_draw_text(88U, 71U, "GYR Z", TFT_WHITE, TFT_NAVY, 1U);
    draw_signed(128U, 71U, telemetry_data.gyro_z);
    tft_draw_text(8U, 94U, "PROBKI", TFT_WHITE, TFT_NAVY, 1U);
    tft_draw_uint(50U, 94U, telemetry_samples, TFT_YELLOW, TFT_NAVY, 1U);
    tft_draw_text(8U, 110U, "< MENU  SW TEST",
                  TFT_GREEN, TFT_NAVY, 1U);
}

static void draw_flight_calibration(void)
{
    draw_header("KALIBRACJA");
    tft_draw_text(17U, 38U, "POLOZ PLYTKE PLASKO", TFT_YELLOW, TFT_NAVY, 1U);
    tft_draw_text(23U, 55U, "NIE RUSZAJ CZUJNIKA", TFT_WHITE, TFT_NAVY, 1U);
    tft_draw_text(35U, 78U, "PROBKI", TFT_CYAN, TFT_NAVY, 1U);
    tft_draw_uint(78U, 78U, flight_calibration_samples,
                  TFT_YELLOW, TFT_NAVY, 1U);
    tft_draw_text(93U, 78U, "/12", TFT_CYAN, TFT_NAVY, 1U);
}

static void draw_paint(void)
{
    draw_header("PAINT");
    tft_draw_text(7U, 21U, "MPU6050: PRZECHYLAJ PLYTKE",
                  TFT_WHITE, TFT_NAVY, 1U);
    tft_draw_text(8U, 112U, "STOL = SRODEK  SW PAUZA",
                  TFT_GREEN, TFT_NAVY, 1U);
    tft_fill_rect(paint_x - 2U, paint_y - 2U, 5U, 5U,
                  paint_color == 0U ? TFT_CYAN :
                  paint_color == 1U ? TFT_YELLOW : TFT_ORANGE);
}

static void draw_balance(void)
{
    draw_header("BALANS");
    tft_draw_text(7U, 21U, "UTRZYMAJ KROPKE W CELU",
                  TFT_WHITE, TFT_NAVY, 1U);
    tft_draw_text(8U, 112U, "PRZECHYLAJ  SW PAUZA",
                  TFT_GREEN, TFT_NAVY, 1U);
    tft_fill_rect(balance_target_x - BALANCE_TARGET_RADIUS,
                  balance_target_y - BALANCE_TARGET_RADIUS,
                  BALANCE_TARGET_RADIUS * 2U + 1U,
                  BALANCE_TARGET_RADIUS * 2U + 1U, TFT_GREEN);
    tft_fill_rect(balance_target_x - 2U, balance_target_y - 2U, 5U, 5U,
                  TFT_NAVY);
    tft_fill_rect(balance_x - 2U, balance_y - 2U, 5U, 5U, TFT_YELLOW);
    tft_draw_text(7U, 4U, "WYNIK", TFT_WHITE, TFT_PURPLE, 1U);
    tft_draw_uint(43U, 4U, balance_score, TFT_YELLOW, TFT_PURPLE, 1U);
}

static void draw_flight_obstacle(const FlightObstacle *obstacle,
                                 uint16_t color)
{
    uint16_t top_height = obstacle->gap_y - PLAY_TOP;
    uint16_t bottom_y = obstacle->gap_y + obstacle->gap_height;
    tft_fill_rect(obstacle->x, PLAY_TOP, 10U, top_height, color);
    tft_fill_rect(obstacle->x, bottom_y, 10U,
                  PLAY_BOTTOM - bottom_y, color);
}

static void draw_flight_plane(uint16_t color)
{
    tft_fill_rect(player_x - 4U, player_y - 2U, 9U, 5U, color);
    tft_fill_rect(player_x - 1U, player_y - 4U, 5U, 9U, color);
    tft_fill_rect(player_x + 4U, player_y - 1U, 4U, 3U, color);
}

static void draw_flight_status(void)
{
    tft_fill_rect(4U, 1U, 152U, 17U, TFT_PURPLE);
    tft_draw_text(7U, 5U, "LOT", TFT_YELLOW, TFT_PURPLE, 1U);
    tft_draw_text(38U, 5U, "WYNIK", TFT_WHITE, TFT_PURPLE, 1U);
    tft_draw_uint(75U, 5U, score, TFT_CYAN, TFT_PURPLE, 1U);
    tft_draw_text(102U, 5U, "MPU", telemetry_ok ? TFT_GREEN : TFT_RED,
                  TFT_PURPLE, 1U);
}

static void redraw_flight(void)
{
    uint8_t index;
    draw_header("SAMOLOT");
    tft_draw_text(6U, 21U, "PRZECHYL: GORA / KIERUNEK", TFT_WHITE,
                  TFT_NAVY, 1U);
    for (index = 0U; index < FLIGHT_OBSTACLES; ++index)
    {
        draw_flight_obstacle(&flight_obstacles[index], TFT_RED);
    }
    draw_flight_plane(TFT_YELLOW);
    draw_flight_status();
}

static void start_flight(uint32_t seed)
{
    screen = GAME_FLIGHT;
    random_state = seed ^ 0xC19F42A7UL;
    frame = 0U;
    score = 0U;
    player_x = 38U;
    player_y = 76U;
    flight_calibrating = 1U;
    flight_calibration_samples = 0U;
    flight_calibration_valid = 0U;
    flight_calibration_sum_y = 0;
    flight_neutral_y = 0;
    draw_flight_calibration();
}

static void finish_flight_calibration(void)
{
    if (flight_calibration_valid != 0U)
    {
        flight_neutral_y = (int16_t)(flight_calibration_sum_y /
                                     flight_calibration_valid);
        flight_neutral_x = (int16_t)(flight_calibration_sum_x /
                                     flight_calibration_valid);
    }
    flight_calibrating = 0U;
    telemetry_ok = flight_calibration_valid != 0U;
    sensor_filtered_x = flight_neutral_x;
    sensor_filtered_y = flight_neutral_y;
    if (screen == GAME_PAINT)
    {
        paint_x = TFT_WIDTH / 2U;
        paint_y = (PLAY_TOP + PLAY_BOTTOM) / 2U;
        paint_color = 0U;
        paint_sensor_cooldown = 0U;
        draw_paint();
    }
    else if (screen == GAME_BALANCE)
    {
        balance_x = TFT_WIDTH / 2U;
        balance_y = (PLAY_TOP + PLAY_BOTTOM) / 2U;
        balance_target_x = balance_x;
        balance_target_y = balance_y;
        balance_time = 0U;
        balance_score = 0U;
        draw_balance();
    }
    else
    {
        uint8_t index;
        for (index = 0U; index < FLIGHT_OBSTACLES; ++index)
        {
            flight_obstacles[index].x = (uint16_t)(170U + index * 45U);
            flight_obstacles[index].gap_height = 35U;
            flight_obstacles[index].gap_y = (uint16_t)(PLAY_TOP + 12U +
                next_random() % (PLAY_BOTTOM - PLAY_TOP - 47U));
        }
        redraw_flight();
    }
}

static void start_paint(void)
{
    screen = GAME_PAINT;
    frame = 0U;
    flight_calibrating = 1U;
    flight_calibration_samples = 0U;
    flight_calibration_valid = 0U;
    flight_calibration_sum_y = 0;
    flight_calibration_sum_x = 0;
    flight_neutral_y = 0;
    flight_neutral_x = 0;
    draw_flight_calibration();
}

static void start_balance(void)
{
    screen = GAME_BALANCE;
    random_state = 0x5EED1234UL;
    flight_calibrating = 1U;
    flight_calibration_samples = 0U;
    flight_calibration_valid = 0U;
    flight_calibration_sum_y = 0;
    flight_calibration_sum_x = 0;
    flight_neutral_y = 0;
    flight_neutral_x = 0;
    draw_flight_calibration();
}

static void update_paint(JoystickState state)
{
    int32_t height_cm = 15;
    int32_t delta_y;
    int32_t delta_x;
    (void)state;

    if (flight_calibrating != 0U)
    {
        telemetry_ok = mpu6050_read(&telemetry_data) == HAL_OK;
        if (telemetry_ok)
        {
            flight_calibration_sum_y += telemetry_data.accel_y;
            flight_calibration_sum_x += telemetry_data.accel_x;
            ++flight_calibration_valid;
        }
        ++flight_calibration_samples;
        draw_flight_calibration();
        if (flight_calibration_samples >= FLIGHT_CALIBRATION_SAMPLES)
        {
            finish_flight_calibration();
        }
        return;
    }

    if (telemetry_ok && mpu6050_read(&telemetry_data) == HAL_OK)
    {
        sensor_filtered_x += ((int32_t)telemetry_data.accel_x -
                              sensor_filtered_x) / 4;
        sensor_filtered_y += ((int32_t)telemetry_data.accel_y -
                              sensor_filtered_y) / 4;
        delta_x = sensor_filtered_x - flight_neutral_x;
        delta_y = sensor_filtered_y - flight_neutral_y;
        if (delta_x > -SENSOR_TILT_DEADZONE &&
            delta_x < SENSOR_TILT_DEADZONE) delta_x = 0;
        if (delta_y > -SENSOR_TILT_DEADZONE &&
            delta_y < SENSOR_TILT_DEADZONE) delta_y = 0;
        {
            int32_t next_x = (int32_t)(TFT_WIDTH / 2U) +
                delta_x * (PLAY_RIGHT - PLAY_LEFT - 8U) / SENSOR_TILT_SCALE;
            if (next_x < PLAY_LEFT + 3U) next_x = PLAY_LEFT + 3U;
            if (next_x > PLAY_RIGHT - 3U) next_x = PLAY_RIGHT - 3U;
            paint_x = (uint16_t)next_x;
        }
        height_cm = 15 + delta_y / 273;
        if (height_cm < 0) height_cm = 0;
        if (height_cm > 30) height_cm = 30;
        paint_y = (uint16_t)(PLAY_BOTTOM -
            height_cm * (PLAY_BOTTOM - PLAY_TOP) / 30);
        if (paint_sensor_cooldown > 0U) --paint_sensor_cooldown;
        if (paint_sensor_cooldown == 0U &&
            (telemetry_data.gyro_z > 18000 || telemetry_data.gyro_z < -18000))
        {
            paint_color = (uint8_t)((paint_color + 1U) % 3U);
            paint_sensor_cooldown = 20U;
        }
        tft_fill_rect(paint_x - 2U, paint_y - 2U, 5U, 5U,
                      paint_color == 0U ? TFT_CYAN :
                      paint_color == 1U ? TFT_YELLOW : TFT_ORANGE);
    }
    ++frame;
}

static void update_balance(void)
{
    int32_t delta_x;
    int32_t delta_y;
    uint16_t distance_x;
    uint16_t distance_y;

    if (flight_calibrating != 0U)
    {
        telemetry_ok = mpu6050_read(&telemetry_data) == HAL_OK;
        if (telemetry_ok)
        {
            flight_calibration_sum_y += telemetry_data.accel_y;
            flight_calibration_sum_x += telemetry_data.accel_x;
            ++flight_calibration_valid;
        }
        ++flight_calibration_samples;
        draw_flight_calibration();
        if (flight_calibration_samples >= FLIGHT_CALIBRATION_SAMPLES)
            finish_flight_calibration();
        return;
    }
    if (mpu6050_read(&telemetry_data) != HAL_OK)
    {
        telemetry_ok = 0U;
        return;
    }
    telemetry_ok = 1U;
    sensor_filtered_x += ((int32_t)telemetry_data.accel_x -
                          sensor_filtered_x) / 4;
    sensor_filtered_y += ((int32_t)telemetry_data.accel_y -
                          sensor_filtered_y) / 4;
    delta_x = sensor_filtered_x - flight_neutral_x;
    delta_y = sensor_filtered_y - flight_neutral_y;
    if (delta_x > -SENSOR_TILT_DEADZONE && delta_x < SENSOR_TILT_DEADZONE)
        delta_x = 0;
    if (delta_y > -SENSOR_TILT_DEADZONE && delta_y < SENSOR_TILT_DEADZONE)
        delta_y = 0;
    {
        int32_t next_x = (int32_t)(TFT_WIDTH / 2U) +
            delta_x * (PLAY_RIGHT - PLAY_LEFT - 8U) / SENSOR_TILT_SCALE;
        int32_t next_y = (int32_t)((PLAY_TOP + PLAY_BOTTOM) / 2U) -
            delta_y * (PLAY_BOTTOM - PLAY_TOP - 8U) / SENSOR_TILT_SCALE;
        if (next_x < PLAY_LEFT + 3U) next_x = PLAY_LEFT + 3U;
        if (next_x > PLAY_RIGHT - 3U) next_x = PLAY_RIGHT - 3U;
        if (next_y < PLAY_TOP + 3U) next_y = PLAY_TOP + 3U;
        if (next_y > PLAY_BOTTOM - 3U) next_y = PLAY_BOTTOM - 3U;
        balance_x = (uint16_t)next_x;
        balance_y = (uint16_t)next_y;
    }
    distance_x = balance_x > balance_target_x ?
                 balance_x - balance_target_x : balance_target_x - balance_x;
    distance_y = balance_y > balance_target_y ?
                 balance_y - balance_target_y : balance_target_y - balance_y;
    if (distance_x <= BALANCE_TARGET_RADIUS &&
        distance_y <= BALANCE_TARGET_RADIUS) ++balance_score;
    ++balance_time;
    if ((balance_time % 100U) == 0U)
    {
        balance_target_x = (uint16_t)(PLAY_LEFT + 10U +
            next_random() % (PLAY_RIGHT - PLAY_LEFT - 20U));
        balance_target_y = (uint16_t)(PLAY_TOP + 10U +
            next_random() % (PLAY_BOTTOM - PLAY_TOP - 20U));
    }
    draw_balance();
}

static void update_flight(JoystickState state)
{
    uint8_t index;
    uint16_t old_x = player_x;
    uint16_t old_y = player_y;
    int32_t tilt_y;

    if (flight_calibrating != 0U)
    {
        telemetry_ok = mpu6050_read(&telemetry_data) == HAL_OK;
        if (telemetry_ok)
        {
            flight_calibration_sum_y += telemetry_data.accel_y;
            flight_calibration_sum_x += telemetry_data.accel_x;
            ++flight_calibration_valid;
        }
        ++flight_calibration_samples;
        draw_flight_calibration();
        if (flight_calibration_samples >= FLIGHT_CALIBRATION_SAMPLES)
        {
            finish_flight_calibration();
        }
        return;
    }

    ++frame;
    if ((frame & 1U) == 0U)
    {
        telemetry_ok = mpu6050_read(&telemetry_data) == HAL_OK;
    }
    tilt_y = (int32_t)telemetry_data.accel_y - flight_neutral_y;
    if (!telemetry_ok)
    {
        if (direction_is_up(state.direction)) tilt_y = -12000;
        else if (direction_is_down(state.direction)) tilt_y = 12000;
        else tilt_y = 0;
    }
    if (state.direction == JOY_LEFT || state.direction == JOY_UP_LEFT ||
        state.direction == JOY_DOWN_LEFT) player_x -= 2U;
    else if (state.direction == JOY_RIGHT || state.direction == JOY_UP_RIGHT ||
             state.direction == JOY_DOWN_RIGHT) player_x += 2U;
    if (tilt_y < -3500) player_y -= 2U;
    else if (tilt_y > 3500) player_y += 2U;
    if (player_x < PLAY_LEFT + 6U) player_x = PLAY_LEFT + 6U;
    if (player_x > PLAY_RIGHT - 6U) player_x = PLAY_RIGHT - 6U;
    if (player_y < PLAY_TOP + 6U) player_y = PLAY_TOP + 6U;
    if (player_y > PLAY_BOTTOM - 6U) player_y = PLAY_BOTTOM - 6U;

    draw_flight_plane(TFT_BLACK);
    for (index = 0U; index < FLIGHT_OBSTACLES; ++index)
    {
        draw_flight_obstacle(&flight_obstacles[index], TFT_BLACK);
        if (flight_obstacles[index].x > PLAY_LEFT)
            --flight_obstacles[index].x;
        else
        {
            flight_obstacles[index].x = PLAY_RIGHT + 20U;
            flight_obstacles[index].gap_height =
                (uint16_t)(29U + next_random() % 15U);
            flight_obstacles[index].gap_y = (uint16_t)(PLAY_TOP + 8U +
                next_random() % (PLAY_BOTTOM - PLAY_TOP -
                                  flight_obstacles[index].gap_height - 8U));
            ++score;
        }
        draw_flight_obstacle(&flight_obstacles[index], TFT_RED);
        if (flight_obstacles[index].x < player_x + 5U &&
            flight_obstacles[index].x + 10U > player_x - 4U &&
            (player_y - 3U < flight_obstacles[index].gap_y ||
             player_y + 3U > flight_obstacles[index].gap_y +
                                  flight_obstacles[index].gap_height))
        {
            if (lose_life_or_end() != 0U) start_flight(next_random());
            return;
        }
    }
    if (old_x != player_x || old_y != player_y)
    {
        draw_flight_plane(TFT_YELLOW);
    }
    if ((frame % 33U) == 0U) draw_flight_status();
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
    aim_level = 1U;
    target_radius = 8U;
    start_aim_round();
    draw_header("LABIRYNT");
    tft_draw_text(4U, 21U, "A DO B  SW PAUZA", TFT_WHITE,
                  TFT_NAVY, 1U);
    tft_draw_text(104U, 21U, "POZ", TFT_CYAN, TFT_NAVY, 1U);
    tft_draw_uint(128U, 21U, aim_level, TFT_YELLOW, TFT_NAVY, 1U);
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
    draw_header("LABIRYNT");
    tft_draw_text(4U, 21U, "A DO B  SW PAUZA", TFT_WHITE,
                  TFT_NAVY, 1U);
    tft_draw_text(104U, 21U, "POZ", TFT_CYAN, TFT_NAVY, 1U);
    tft_draw_uint(128U, 21U, aim_level, TFT_YELLOW, TFT_NAVY, 1U);
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
    tft_draw_text(4U, 21U, "BOMBY 4 STRONY  SW", TFT_WHITE,
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
        if (frame % 2U == 0U)
        {
            bombs[index].x = (uint16_t)((int16_t)bombs[index].x + bombs[index].dx);
            bombs[index].y = (uint16_t)((int16_t)bombs[index].y + bombs[index].dy);
        }
        if (bombs[index].x < PLAY_LEFT || bombs[index].x > PLAY_RIGHT ||
            bombs[index].y < PLAY_TOP || bombs[index].y > PLAY_BOTTOM)
        {
            bomb_count--;
            bombs[index] = bombs[bomb_count];
            --index;
            continue;
        }
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
        {
            uint8_t edge = (uint8_t)(next_random() % 4U);
            if (edge == 0U)
            {
                bombs[bomb_count].x = PLAY_LEFT;
                bombs[bomb_count].y = (uint16_t)(PLAY_TOP + next_random() %
                                                 (PLAY_BOTTOM - PLAY_TOP));
                bombs[bomb_count].dx = 1;
                bombs[bomb_count].dy = 0;
            }
            else if (edge == 1U)
            {
                bombs[bomb_count].x = PLAY_RIGHT;
                bombs[bomb_count].y = (uint16_t)(PLAY_TOP + next_random() %
                                                 (PLAY_BOTTOM - PLAY_TOP));
                bombs[bomb_count].dx = -1;
                bombs[bomb_count].dy = 0;
            }
            else if (edge == 2U)
            {
                bombs[bomb_count].x = (uint16_t)(PLAY_LEFT + next_random() %
                                                 (PLAY_RIGHT - PLAY_LEFT));
                bombs[bomb_count].y = PLAY_TOP;
                bombs[bomb_count].dx = 0;
                bombs[bomb_count].dy = 1;
            }
            else
            {
                bombs[bomb_count].x = (uint16_t)(PLAY_LEFT + next_random() %
                                                 (PLAY_RIGHT - PLAY_LEFT));
                bombs[bomb_count].y = PLAY_BOTTOM;
                bombs[bomb_count].dx = 0;
                bombs[bomb_count].dy = -1;
            }
        }
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
    uint8_t index;
    screen = GAME_MENU;
    menu_item = 0U;
    last_menu_direction = JOY_CENTER;
    telemetry_ok = mpu6050_init() == HAL_OK;
    telemetry_identity = 0U;
    if (mpu6050_get_identity(&telemetry_identity) != HAL_OK)
    {
        telemetry_identity = 0U;
    }
    telemetry_frame = 0U;
    telemetry_samples = 0U;
    telemetry_data.accel_x = 0;
    telemetry_data.accel_y = 0;
    telemetry_data.accel_z = 0;
    telemetry_data.gyro_x = 0;
    telemetry_data.gyro_y = 0;
    telemetry_data.gyro_z = 0;
    led_test_item = 0U;
    for (index = 0U; index < 5U; ++index) led_test_state[index] = 1U;
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
                menu_item = (uint8_t)((menu_item +
                            (direction_is_down(state.direction) ? 1U : 3U)) % 4U);
                draw_menu();
            }
            last_menu_direction = state.direction;
        }
        if (button_event != 0U)
        {
            if (menu_item == 0U) start_snake(state.x ^ ((uint32_t)state.y << 16));
            else if (menu_item == 1U) start_dodge(state.x ^ ((uint32_t)state.y << 16));
            else if (menu_item == 2U) start_aim(state.x ^ ((uint32_t)state.y << 16));
            else
            {
                screen = GAME_SENSOR_MENU;
                sensor_menu_item = 0U;
                draw_sensor_menu();
            }
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
                else if (screen == GAME_AIM) redraw_aim();
                else if (screen == GAME_FLIGHT) redraw_flight();
                else if (screen == GAME_PAINT) draw_paint();
                else if (screen == GAME_BALANCE) draw_balance();
                else draw_telemetry();
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
    if (screen == GAME_SENSOR_MENU)
    {
        if (state.direction != last_menu_direction)
        {
            if (direction_is_up(state.direction) ||
                direction_is_down(state.direction))
            {
                sensor_menu_item = (uint8_t)((sensor_menu_item +
                    (direction_is_down(state.direction) ? 1U : 3U)) % 4U);
                draw_sensor_menu();
            }
            last_menu_direction = state.direction;
        }
        if (state.direction == JOY_LEFT || state.direction == JOY_UP_LEFT ||
            state.direction == JOY_DOWN_LEFT)
        {
            game_init();
            return;
        }
        if (button_event != 0U)
        {
            if (sensor_menu_item == 0U)
            {
                screen = GAME_TELEMETRY;
                telemetry_frame = 0U;
                draw_telemetry();
            }
            else if (sensor_menu_item == 1U)
            {
                start_paint();
            }
            else if (sensor_menu_item == 2U)
            {
                start_balance();
            }
            else
            {
                screen = GAME_LED_INFO;
                draw_led_info();
            }
        }
        return;
    }
    if (screen == GAME_LED_INFO)
    {
        if (state.direction != last_menu_direction)
        {
            if (direction_is_up(state.direction) || direction_is_down(state.direction))
            {
                led_test_item = (uint8_t)((led_test_item +
                    (direction_is_down(state.direction) ? 1U : 4U)) % 5U);
                draw_led_info();
            }
            else if (state.direction == JOY_LEFT || state.direction == JOY_RIGHT)
            {
                led_test_state[led_test_item] =
                    led_test_state[led_test_item] == 0U ? 1U : 0U;
                lives_set_led(led_test_item, led_test_state[led_test_item]);
                draw_led_info();
            }
            last_menu_direction = state.direction;
        }
        if (button_event != 0U)
        {
            screen = GAME_SENSOR_MENU;
            draw_sensor_menu();
        }
        return;
    }
    if (screen == GAME_TELEMETRY)
    {
        if (state.direction == JOY_LEFT || state.direction == JOY_UP_LEFT ||
            state.direction == JOY_DOWN_LEFT)
        {
            screen = GAME_SENSOR_MENU;
            draw_sensor_menu();
            return;
        }
        if (button_event != 0U || (++telemetry_frame % 33U) == 0U)
        {
            telemetry_ok = mpu6050_read(&telemetry_data) == HAL_OK;
            if (telemetry_ok) ++telemetry_samples;
            draw_telemetry();
        }
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
    else if (screen == GAME_FLIGHT) update_flight(state);
    else if (screen == GAME_PAINT) update_paint(state);
    else if (screen == GAME_BALANCE) update_balance();
}
