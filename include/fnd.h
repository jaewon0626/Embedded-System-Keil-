#ifndef __FND_H__
#define __FND_H__

#include "stm32f446xx.h"

void Fnd_Initialize(void);
void Fnd_Self_Test(void);
void Fnd_Display(int8_t num, int8_t position, uint32_t time_ms, int dot_flag);
// void fnd_display(void);

#endif
