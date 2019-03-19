#ifndef __HAL_KEY_H__
#define __HAL_KEY_H__


#include "stdint.h"
#include "M051Series.h"

#define     KEY_DEBOUNCE_TIME        50
#define     KEY_TOUCH_LONG           2000
#define     KEY_TOUCH_VLONG          4000

#define     HAL_KEY_NUM_MAX     2

#define HAL_KEY_0    0
#define HAL_KEY_1    1
#define HAL_KEY_2    2
#define HAL_KEY_3    3
#define HAL_KEY_4    4
#define HAL_KEY_5    5       //reserved
#define HAL_KEY_6    6       //reserved
#define HAL_KEY_7    7       //reserved

#define HAL_KEY_EVT_Enter       0x01
#define HAL_KEY_EVT_Leave       0x02
#define HAL_KEY_EVT_Short       0x04
#define HAL_KEY_EVT_Long        0x08
#define HAL_KEY_EVT_VLong       0x10
#define HAL_KEY_EVT_All         (HAL_KEY_EVT_Enter | HAL_KEY_EVT_Leave | HAL_KEY_EVT_Short | HAL_KEY_EVT_Long | HAL_KEY_EVT_VLong)

#define HAL_KEY_STATE_PRESSED   1
#define HAL_KEY_STATE_RELEASED  0

#define HAL_STATE_KEY0()        (P45 ? HAL_KEY_STATE_RELEASED : HAL_KEY_STATE_PRESSED)
#define HAL_STATE_KEY1()        (P32 ? HAL_KEY_STATE_RELEASED : HAL_KEY_STATE_PRESSED)
#define HAL_STATE_KEY2()        HAL_KEY_STATE_RELEASED//(P45 ? HAL_KEY_STATE_RELEASED : HAL_KEY_STATE_PRESSED)
#define HAL_STATE_KEY3()        HAL_KEY_STATE_RELEASED//(P25 ? HAL_KEY_STATE_RELEASED : HAL_KEY_STATE_PRESSED)
#define HAL_STATE_KEY4()        HAL_KEY_STATE_RELEASED//(P26 ? HAL_KEY_STATE_RELEASED : HAL_KEY_STATE_PRESSED)


typedef struct {
    uint8_t keyId;
    uint8_t keyEvt;
} HalKeyMsg_t;

extern void HalKeyInit(void);
extern void HalKeyEvtConfig(uint8_t keyId, uint8_t keyEvt);
extern void HalKeyEvtHandler(void);
extern uint8_t HalKeyRead(void);
extern void HalKeyIsr(void);
extern void HalKeyPoll(void);       //for no-interrupt situation

#endif /* __HAL_KEY_H__ */

