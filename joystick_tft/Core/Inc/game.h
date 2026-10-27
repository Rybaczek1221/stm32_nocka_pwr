#ifndef GAME_H
#define GAME_H

#include "joystick.h"

void game_show_menu(void);
void game_start(void);
void game_update(JoystickState state);

#endif
