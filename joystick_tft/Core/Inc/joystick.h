#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "main.h"

typedef enum
{
    JOY_CENTER = 0,
    JOY_UP,
    JOY_UP_RIGHT,
    JOY_RIGHT,
    JOY_DOWN_RIGHT,
    JOY_DOWN,
    JOY_DOWN_LEFT,
    JOY_LEFT,
    JOY_UP_LEFT
} JoystickDirection;

typedef struct
{
    uint16_t x;
    uint16_t y;
    JoystickDirection direction;
    uint8_t pressed;
} JoystickState;

JoystickState joystick_read(void);

#endif
