#ifndef LIVES_H
#define LIVES_H

#include "main.h"

void lives_init(void);
void lives_reset(void);
uint8_t lives_lose(void);
uint8_t lives_get(void);
void lives_set_led(uint8_t index, uint8_t enabled);

#endif
