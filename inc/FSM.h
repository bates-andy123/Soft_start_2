#ifndef FSM_H_
#define FSM_H_

#include <UART.h>
#include "stm32f0xx.h"
#include "Buffer.h"

Buffer inputBuffer;

/* Public function prototypes -----------------------------------------------*/

void FSM_Init();

#endif // FSM_H_

