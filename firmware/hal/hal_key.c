#include "hal_key.h"
#include "hal_assert.h"
#include "hal_drivers.h"
#include "osal.h"
#include "osal_cbtimer.h"

#include "app_lock.h"

#define     KEY_TID_EN_LONG          0x01
#define     KEY_TID_EN_VLONG         0x02

#define     KEY_BIT_MASK(x)          (1<<x)


typedef struct {
    uint8_t tidLong;
    uint8_t tidVLong;
    uint8_t tidEn;
    uint8_t keyEvtEn;
} KeyProfile_t;

typedef struct {
    KeyProfile_t keyProfile[HAL_KEY_NUM_MAX];
    uint8_t keyState;
} KCB_t; //Key Control Body

static KCB_t kcb;

static void onButtonTouchLong(uint8_t *pData)
{
    uint8_t keyId;
    AppLockMsgKey_t *pMsg;
    
    keyId = (uint8_t)((uint32_t)pData);
    //clear tidEn firstly
    kcb.keyProfile[keyId].tidEn &= ~KEY_TID_EN_LONG;
    //HAL_ASSERT(kcb.keyProfile[keyId].keyEvtEn & HAL_KEY_EVT_Long);

    if(kcb.keyProfile[keyId].keyEvtEn & HAL_KEY_EVT_Long)
    {
        //send touch long msg
        pMsg = (AppLockMsgKey_t *)osal_msg_allocate(sizeof(AppLockMsgKey_t));
        HAL_ASSERT(pMsg != NULL);
        pMsg->head = APP_LOCK_MSG_HEAD_KeyMsg;
        pMsg->keyMsg.keyEvt = HAL_KEY_EVT_Long;
        pMsg->keyMsg.keyId = keyId;
        osal_msg_send(AppLockTaskID, (uint8_t *) pMsg);
    }
}
static void onButtonTouchVLong(uint8_t *pData)
{
    //clear tidEn firstly
    uint8_t keyId;
    AppLockMsgKey_t *pMsg;
    
    keyId = (uint8_t)((uint32_t)pData);
    //clear tidEn firstly
    kcb.keyProfile[keyId].tidEn &= ~KEY_TID_EN_VLONG;
    //HAL_ASSERT(kcb.keyProfile[keyId].keyEvtEn & HAL_KEY_EVT_VLong);

    if(kcb.keyProfile[keyId].keyEvtEn & HAL_KEY_EVT_VLong)
    {
        //send touch vlong msg
        pMsg = (AppLockMsgKey_t *)osal_msg_allocate(sizeof(AppLockMsgKey_t));
        HAL_ASSERT(pMsg != NULL);
        pMsg->head = APP_LOCK_MSG_HEAD_KeyMsg;
        pMsg->keyMsg.keyEvt = HAL_KEY_EVT_VLong;
        pMsg->keyMsg.keyId = keyId;
        osal_msg_send(AppLockTaskID, (uint8_t *) pMsg);
    }
}

static void HalKeyHandle(uint8_t keyState)
{
    uint8_t i;
    uint8_t keyChanged;
    AppLockMsgKey_t *pMsg;
    
    keyChanged = keyState ^ kcb.keyState;
    
    for(i = 0; i < HAL_KEY_NUM_MAX; i++)
    {
        //check if button status changed
        if(keyChanged & KEY_BIT_MASK(i))
        {
            if(keyState & KEY_BIT_MASK(i))
            {
                //touch enter
                //if(kcb.keyProfile[i].keyEvtEn & HAL_KEY_EVT_Long)
                {
                    HAL_ASSERT(osal_CbTimerStart(onButtonTouchLong, (uint8_t *)i, KEY_TOUCH_LONG, &kcb.keyProfile[i].tidLong) == SUCCESS);
                    kcb.keyProfile[i].tidEn |= KEY_TID_EN_LONG;
                }
                if(kcb.keyProfile[i].keyEvtEn & HAL_KEY_EVT_VLong)
                {
                    HAL_ASSERT(osal_CbTimerStart(onButtonTouchVLong, (uint8_t *)i, KEY_TOUCH_VLONG, &kcb.keyProfile[i].tidVLong) == SUCCESS);
                    kcb.keyProfile[i].tidEn |= KEY_TID_EN_VLONG;
                }
                
                if(kcb.keyProfile[i].keyEvtEn & HAL_KEY_EVT_Enter)
                {
                    //send touch enter msg here
                    pMsg = (AppLockMsgKey_t *)osal_msg_allocate(sizeof(AppLockMsgKey_t));
                    HAL_ASSERT(pMsg != NULL);
                    pMsg->head = APP_LOCK_MSG_HEAD_KeyMsg;
                    pMsg->keyMsg.keyEvt = HAL_KEY_EVT_Enter;
                    pMsg->keyMsg.keyId = i;
                    osal_msg_send(AppLockTaskID, (uint8_t *) pMsg);
                }
            }
            else
            {
                //touch leave
                
                //check if long touch event happened
                if(kcb.keyProfile[i].tidEn & KEY_TID_EN_LONG)
                {
                    HAL_ASSERT(osal_CbTimerStop(kcb.keyProfile[i].tidLong) == SUCCESS);
                    kcb.keyProfile[i].tidEn &= ~KEY_TID_EN_LONG;
                    //short touch
                    if(kcb.keyProfile[i].keyEvtEn & HAL_KEY_EVT_Short)
                    {
                        //send touch enter msg here
                        pMsg = (AppLockMsgKey_t *)osal_msg_allocate(sizeof(AppLockMsgKey_t));
                        HAL_ASSERT(pMsg != NULL);
                        pMsg->head = APP_LOCK_MSG_HEAD_KeyMsg;
                        pMsg->keyMsg.keyEvt = HAL_KEY_EVT_Short;
                        pMsg->keyMsg.keyId = i;
                        osal_msg_send(AppLockTaskID, (uint8_t *) pMsg);
                    }
                }

                //check if vlong touch event happened
                if(kcb.keyProfile[i].tidEn & KEY_TID_EN_VLONG)
                {
                    HAL_ASSERT(osal_CbTimerStop(kcb.keyProfile[i].tidVLong) == SUCCESS);
                    kcb.keyProfile[i].tidEn &= ~KEY_TID_EN_VLONG;
                }

                //trigger touch leave event
                if(kcb.keyProfile[i].keyEvtEn & HAL_KEY_EVT_Leave)
                {
                    //send touch leave msg here
                    pMsg = (AppLockMsgKey_t *)osal_msg_allocate(sizeof(AppLockMsgKey_t));
                    HAL_ASSERT(pMsg != NULL);
                    pMsg->head = APP_LOCK_MSG_HEAD_KeyMsg;
                    pMsg->keyMsg.keyEvt = HAL_KEY_EVT_Leave;
                    pMsg->keyMsg.keyId = i;
                    osal_msg_send(AppLockTaskID, (uint8_t *) pMsg);
                }
            }
        }
    }

    kcb.keyState = keyState;
}

extern void HalKeyInit(void)
{
    osal_memset(&kcb, 0x00, sizeof(KCB_t));
}

extern void HalKeyEvtConfig(uint8_t keyId, uint8_t keyEvt)
{
    HAL_ASSERT(keyId < HAL_KEY_NUM_MAX);
    kcb.keyProfile[keyId].keyEvtEn = keyEvt;
}

extern void HalKeyEvtHandler(void)
{
    HalKeyHandle(HalKeyRead());
}

extern uint8_t HalKeyRead(void)
{
    uint8_t keyState = 0x00;
    
    keyState |= HAL_STATE_KEY0();
    keyState |= HAL_STATE_KEY1()<<1;
    keyState |= HAL_STATE_KEY2()<<2;
    keyState |= HAL_STATE_KEY3()<<3;
    keyState |= HAL_STATE_KEY4()<<4;
    return keyState;
}

extern void HalKeyIsr(void)
{
    HAL_ASSERT(osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, KEY_DEBOUNCE_TIME) == SUCCESS);
}

extern void HalKeyPoll(void)
{
    static uint8_t preKeyState;
    uint8_t curKeyState;
    
    curKeyState = HalKeyRead();
    if(curKeyState != preKeyState)
    {
        HalKeyIsr();
        preKeyState = curKeyState;
    }
}



