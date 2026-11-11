#ifndef GAME_H
#define GAME_H

#include "joystick.h"

typedef enum
{
    GAME_MENU = 0,
    GAME_SNAKE,
    GAME_DODGE,
    GAME_AIM,
    GAME_TELEMETRY,
    GAME_FLIGHT,
    GAME_PAINT,
    GAME_BALANCE,
    GAME_SENSOR_MENU,
    GAME_LED_INFO,
    GAME_PAUSE,
    GAME_OVER
} GameScreen;

void game_init(void);
void game_update(JoystickState state, uint32_t button_event);

#endif
