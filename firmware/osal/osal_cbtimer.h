/******************************************************************************

 @file  osal_cbtimer.h

 @brief This file contains the Callback Timer definitions.

 Group: WCS, BTS
 Target Device: CC2540, CC2541

 ******************************************************************************
 
 Copyright (c) 2008-2016, Texas Instruments Incorporated
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
 PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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

#ifndef OSAL_CBTIMER_H
#define OSAL_CBTIMER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
//#include "bcomdef.h"
#include "osal_config.h"
/*********************************************************************
 * CONSTANTS
 */
// Invalid timer id
#define INVALID_TIMER_ID                           0xFF

// Timed out timer
#define TIMEOUT_TIMER_ID                           0xFE

/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * MACROS
 */
#if ( OSAL_CBTIMER_NUM_TASKS == 0 )
  #error Callback Timer module should not be included (no callback timer is needed)!
#elif ( OSAL_CBTIMER_NUM_TASKS == 1 )
  #define OSAL_CBTIMER_PROCESS_EVENT( a )          ( a )
#elif ( OSAL_CBTIMER_NUM_TASKS == 2 )
  #define OSAL_CBTIMER_PROCESS_EVENT( a )          ( a ), ( a )
#else
  #error Maximum of 2 callback timer tasks are supported! Modify it here.
#endif

/*********************************************************************
 * TYPEDEFS
 */

// Callback Timer function prototype. Callback function will be called
// when the associated timer expires.
//
// pData - pointer to data registered with timer
//
typedef void (*pfnCbTimer_t)( uint8_t *pData );

/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Callback Timer task initialization function.
 */
extern void osal_CbTimerInit( uint8_t taskId );

/*
 * Callback Timer task event processing function.
 */
extern uint16_t osal_CbTimerProcessEvent( uint8_t  taskId,
                                        uint16_t events );

/*
 * Function to start a timer to expire in n mSecs.
 */
extern Status_t osal_CbTimerStart( pfnCbTimer_t  pfnCbTimer,
                                   uint8_t        *pData,
                                   uint32_t        timeout,
                                   uint8_t        *pTimerId );

/*
 * Function to start a timer to expire in n mSecs, then reload.
 */
extern Status_t osal_CbTimerStartReload( pfnCbTimer_t  pfnCbTimer,
                                         uint8_t        *pData,
                                         uint32_t        timeout,
                                         uint8_t        *pTimerId );

/*
 * Function to update a timer that has already been started.
 */
extern Status_t osal_CbTimerUpdate( uint8_t  timerId,
                                    uint32_t timeout );

/*
 * Function to stop a timer that has already been started.
 */
extern Status_t osal_CbTimerStop( uint8_t timerId );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CBTIMER_H */
