/******************************************************************************

 @file  hal_drivers.c

 @brief This file contains the interface to the Drivers Service.

 Group: WCS, BTS
 Target Device: CC2540, CC2541

 ******************************************************************************
 
 Copyright (c) 2005-2016, Texas Instruments Incorporated
 All rights reserved.

 IMPORTANT: Your use of this Software is limited to those specific rights
 granted under the terms of a software license agreement between the user
 who downloaded the software, his/her employer (which must be your employer)
 and Texas Instruments Incorporated (the "License"). You may not use this
 Software unless you agree to abide by the terms of the License. The License
 limits your use, and you acknowledge, that the Software may not be modified,
 copied or distributed unless embedded on a Texas Instruments microcontroller
 or used solely and exclusively in conjunction with a Texas Instruments radio
 frequency transceiver, which is integrated into your product. Other than for
 the foregoing purpose, you may not use, reproduce, copy, prepare derivative
 works of, modify, distribute, perform, display or sell this Software and/or
 its documentation for any purpose.

 YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
 PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
 NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
 TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
 NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
 LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
 INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
 OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
 OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
 (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

 Should you have any questions regarding your right to use this Software,
 contact Texas Instruments Incorporated at www.TI.com.

 ******************************************************************************
 Release Name: ble_sdk_1.4.2.2
 Release Date: 2016-06-09 06:57:09
 *****************************************************************************/

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include "osal.h"
#include "hal_drivers.h"
#include "hal_terminal.h"
#include "hal_atcmdsio.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_buzzer.h"
#include "hal_devid.h"
#include "hal_flash.h"
#include "hal_assert.h"
#if defined POWER_SAVING
#include "OSAL_PwrMgr.h"
#endif

static void hal_ProcessOsalMsg(HalDriverMsg_t *pMsg);
/**************************************************************************************************
 *                                         CONSTANTS
 **************************************************************************************************/
/**************************************************************************************************
 *                                      GLOBAL VARIABLES
 **************************************************************************************************/
uint8_t Hal_TaskID;
extern void HalLedUpdate (void);    /* Notes: This for internal only so it shouldn't be in hal_led.h */

/**************************************************************************************************
 * @fn      Hal_Init
 *
 * @brief   Hal Initialization function.
 *
 * @param   task_id - Hal TaskId
 *
 * @return  None
 **************************************************************************************************/
void Hal_Init( uint8_t task_id )
{
  /* Register task ID */
  Hal_TaskID = task_id;
}

/**************************************************************************************************
 * @fn      Hal_DriverInit
 *
 * @brief   Initialize HW - These need to be initialized before anyone.
 *
 * @param   task_id - Hal TaskId
 *
 * @return  None
 **************************************************************************************************/
void HalDriverInit (void)
{
  /* TIMER */
  /* ADC */
  /* DMA */
  /* AES */
  /* LCD */

  /* LED */
  //HalLedInit();

  /* UART */
  //HalUARTInit();
    
    HalLedInit();
    //HalWiFiInit();
    HalDevIdInit();
    
    HalTerminalInit();
    HalATCmdSIOInit();
    
    HalKeyInit();
    HalKeyEvtConfig(HAL_KEY_0, HAL_KEY_EVT_Short | HAL_KEY_EVT_Long);
    HalKeyEvtConfig(HAL_KEY_1, HAL_KEY_EVT_Enter | HAL_KEY_EVT_Leave);

    HalBuzzerInit();
  
  /* SPI */

  /* HID */
}

/**************************************************************************************************
 * @fn      Hal_ProcessEvent
 *
 * @brief   Hal Process Event
 *
 * @param   task_id - Hal TaskId
 *          events - events
 *
 * @return  None
 **************************************************************************************************/
uint16_t Hal_ProcessEvent( uint8_t task_id, uint16_t events )
{
    HalDriverMsg_t *msgPtr;

    if ( events & SYS_EVENT_MSG )
    {
        msgPtr = (HalDriverMsg_t *)osal_msg_receive( task_id );

        while (msgPtr)
        {
          /* Process message */
          hal_ProcessOsalMsg(msgPtr);
          /* De-allocate */
          osal_msg_deallocate( (uint8_t *)msgPtr );
          /* Next */
          msgPtr = (HalDriverMsg_t *)osal_msg_receive( task_id );
        }
        return events ^ SYS_EVENT_MSG;
    }

    if( events & HAL_UART1_RX_DATA_EVT )
    {
        HalATCmdSIOPollRx();
        return events ^ HAL_UART1_RX_DATA_EVT;
    }

    if( events & HAL_UART1_TX_DATA_EVT )
    {
        HalATCmdSIOPollTx();
        return events ^ HAL_UART1_TX_DATA_EVT;
    }

    if( events & HAL_UART0_RX_DATA_EVT )
    {
        HalTerminalPollRx();
        return events ^ HAL_UART0_RX_DATA_EVT;
    }

    if( events & HAL_UART0_TX_DATA_EVT )
    {
        HalTerminalPollTx();
        return events ^ HAL_UART0_TX_DATA_EVT;
    }

    if( events & HAL_BUZZER_UPDATE_EVENT )
    {
        HalBuzzerUpdate();
        return events ^ HAL_BUZZER_UPDATE_EVENT;
    }
    
    if ( events & HAL_LED_BLINK_EVENT )
    {
#if (defined (BLINK_LEDS)) && (HAL_LED == TRUE)
        HalLedUpdate();
#endif /* BLINK_LEDS && HAL_LED */
        return events ^ HAL_LED_BLINK_EVENT;
    }

    if( events & HAL_KEY_EVENT )
    {
        HalKeyEvtHandler();
        return events ^ HAL_KEY_EVENT;
    }

#if defined POWER_SAVING
    if ( events & HAL_SLEEP_TIMER_EVENT )
    {
        halRestoreSleepLevel();
        return events ^ HAL_SLEEP_TIMER_EVENT;
    }

    if ( events & HAL_PWRMGR_HOLD_EVENT )
    {
        (void)osal_pwrmgr_task_state(Hal_TaskID, PWRMGR_HOLD);

        (void)osal_stop_timerEx(Hal_TaskID, HAL_PWRMGR_CONSERVE_EVENT);
        (void)osal_clear_event(Hal_TaskID, HAL_PWRMGR_CONSERVE_EVENT);

        return (events & ~(HAL_PWRMGR_HOLD_EVENT | HAL_PWRMGR_CONSERVE_EVENT));
    }

    if ( events & HAL_PWRMGR_CONSERVE_EVENT )
    {
        (void)osal_pwrmgr_task_state(Hal_TaskID, PWRMGR_CONSERVE);
        return events ^ HAL_PWRMGR_CONSERVE_EVENT;
    }
#endif

  return 0;
}

/**************************************************************************************************
 * @fn      Hal_ProcessPoll
 *
 * @brief   This routine will be called by OSAL to poll UART, TIMER...
 *
 * @param   task_id - Hal TaskId
 *
 * @return  None
 **************************************************************************************************/
void Hal_ProcessPoll ()
{
#if defined( POWER_SAVING )
    /* Allow sleep before the next OSAL event loop */
    ALLOW_SLEEP_MODE();
#endif


    //HalWiFiPoll();
    HalKeyPoll();
}

static void hal_ProcessOsalMsg(HalDriverMsg_t *pMsg)
{
    switch (pMsg->head)
    {
        default:
            HAL_ASSERT_FORCED();
        break;
    }
        
}

/**************************************************************************************************
**************************************************************************************************/

