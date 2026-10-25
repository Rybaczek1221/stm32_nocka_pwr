#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdint.h>
#include "joystick.h"

void led_matrix_init(void);
void led_matrix_show(JoystickDirection direction);
void led_matrix_all(uint32_t on);
void led_matrix_startup_signal(void);

#endif
