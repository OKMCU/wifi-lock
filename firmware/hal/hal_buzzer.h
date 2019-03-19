#ifndef __HAL_BUZZER_H__
#define __HAL_BUZZER_H__


#include "stdint.h"
#include "M051Series.h"

#define HAL_BUZZER_TIME_SHORT   300
#define HAL_BUZZER_TIME_LONG    1000
#define HAL_BUZZER_TIME_VLONG   2000

extern void HalBuzzerInit(void);
extern void HalBuzzerBeep(uint16_t on_time, uint16_t off_time, uint8_t beep_cnt);
extern void HalBuzzerUpdate(void);

#endif /* __HAL_BUZZER_H__ */

