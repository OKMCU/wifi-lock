#include "hal_buzzer.h"
#include "hal_assert.h"
#include "hal_drivers.h"
#include "osal.h"

//static uint16_t onTime;
//static uint16_t offTime;
//static uint8_t beepCnt;
typedef struct {
    
    uint16_t on_time;
    uint16_t off_time;
    uint8_t beep_cnt;
    uint8_t on_cnt;
    uint8_t off_cnt;
    
} HalBuzzerCtrl_t;

static HalBuzzerCtrl_t *pBuzzerCtrl;

static void halBuzzerOn(void)
{
    /* Enable PWM0 output */
    PWM_ConfigOutputChannel(PWMA, PWM_CH0, 2000, 50);
    PWM_EnableOutput(PWMA, 0x1);
    PWM_Start(PWMA, 0x1);
    
}

static void halBuzzerOff(void)
{
    PWM_Stop(PWMA, 0x1);
    /* Enable PWM0 output */
    PWM_DisableOutput(PWMA, 0x1);
    //SYS->P2_MFP &= ~(SYS_MFP_P20_PWM0);
}

extern void HalBuzzerInit(void)
{
    //GPIO_SetMode(P2, BIT0 , GPIO_PMD_OUTPUT);
    
    pBuzzerCtrl = NULL;
}

extern void HalBuzzerBeep(uint16_t on_time, uint16_t off_time, uint8_t beep_cnt)
{
    //Reset buzzer to off state and reset variables.
    if( pBuzzerCtrl != NULL )
    {
        osal_mem_free(pBuzzerCtrl);
        pBuzzerCtrl = NULL;
    }
    halBuzzerOff();
    osal_clear_event( Hal_TaskID, HAL_BUZZER_UPDATE_EVENT );
    osal_stop_timerEx( Hal_TaskID, HAL_BUZZER_UPDATE_EVENT );

    if( on_time > 0 )
    {
        if( beep_cnt > 0 )
        {
            pBuzzerCtrl = osal_mem_alloc( sizeof(HalBuzzerCtrl_t) );
            HAL_ASSERT( pBuzzerCtrl != NULL );
            osal_memset( pBuzzerCtrl, 0x00, sizeof(HalBuzzerCtrl_t) );
            
            pBuzzerCtrl->on_time = on_time;
            pBuzzerCtrl->off_time = off_time;
            pBuzzerCtrl->beep_cnt = off_time == 0 ? 1 : beep_cnt;   //force beep_cnt to 1 if off_time is not available
            osal_set_event( Hal_TaskID, HAL_BUZZER_UPDATE_EVENT );
        }
        else
        {
            halBuzzerOn();
        }
    }
    
}

extern void HalBuzzerUpdate(void)
{
    HAL_ASSERT( pBuzzerCtrl != NULL );
    HAL_ASSERT( pBuzzerCtrl->on_time > 0 );
    HAL_ASSERT( pBuzzerCtrl->beep_cnt > 0 );

    if( pBuzzerCtrl->on_cnt <= pBuzzerCtrl->off_cnt )
    {
        halBuzzerOn();
        pBuzzerCtrl->on_cnt++;
        osal_start_timerEx( Hal_TaskID, HAL_BUZZER_UPDATE_EVENT, pBuzzerCtrl->on_time );
    }
    else
    {
        halBuzzerOff();
        pBuzzerCtrl->off_cnt++;
        
        if( pBuzzerCtrl->on_cnt >= pBuzzerCtrl->beep_cnt )
        {
            //beep task end
            osal_mem_free( pBuzzerCtrl );
            pBuzzerCtrl = NULL;
        }
        else
        {
            //prepare next beep event
            osal_start_timerEx( Hal_TaskID, HAL_BUZZER_UPDATE_EVENT, pBuzzerCtrl->off_time );
        }
    }
    
}






