/******************************************************************************

 @file  osal_config.h

 @brief Defines stuff for EVALuation boards
        This file targets the Chipcon CC2540DB/CC2540EB

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
 Release Date: 2016-06-09 06:57:10
 *****************************************************************************/

#ifndef OSAL_CONFIG_H
#define OSAL_CONFIG_H

#include "hal_mcu.h"
//#include "hal_sleep.h"
#include "osal.h"

/*********************************************************************
 */
// Internal (MCU) RAM addresses
//#define MCU_RAM_BEG 0x0100
//#define MCU_RAM_END RAMEND
//#define MCU_RAM_LEN (MCU_RAM_END - MCU_RAM_BEG + 1)

// Memory Allocation Heap
#define MAXMEMHEAP 2560  // Typically, 0.70-1.50K
#define OSAL_CBTIMER_NUM_TASKS 1
#define OSAL_FIFO_PAGE_SIZE   64
// Initialization levels
//#define OB_COLD  0
//#define OB_WARM  1
//#define OB_READY 2

//#define SystemResetSoft()   while(1)

//typedef struct
//{
//  osal_event_hdr_t hdr;
//  uint8_t             state; // shift
//  uint8_t             keys;  // keys
//} keyChange_t;

// Timer clock and power-saving definitions
//#define TIMER_DECR_TIME    1  // 1ms - has to be matched with TC_OCC
//#define RETUNE_THRESHOLD   1  // Threshold for power saving algorithm

/* OSAL timer defines */
//#define TICK_TIME   1000   /* Timer per tick - in micro-sec */
//#define TICK_COUNT  1
//#define OSAL_TIMER  HAL_TIMER_3

//extern void _itoa(uint16_t num, uint8_t *buf, uint8_t radix);

//#ifndef RAMEND
//#define RAMEND 0x1000
//#endif

///* Tx and Rx buffer size defines used by SPIMgr.c */
//#define MT_UART_THRESHOLD    5
//#define MT_UART_TX_BUFF_MAX  170
//#define MT_UART_RX_BUFF_MAX  128
//#define MT_UART_IDLE_TIMEOUT 5

/* system restart and boot loader used from MTEL.c */
// Restart system from absolute beginning
// Disables interrupts, forces WatchDog reset
//#define SystemReset()      while(1) //HAL_SYSTEM_RESET();

//#define BootLoader()       while(1)

/* Reset reason for reset indication */
//#define ResetReason()       0x00    //((SLEEPSTA >> 3) & 0x03)

/* sleep macros required by OSAL_PwrMgr.c */
//#define SLEEP_DEEP                  0             /* value not used */
//#define SLEEP_LITE                  0             /* value not used */
//#define MIN_SLEEP_TIME              14            /* minimum time to sleep */
//#define OSAL_SET_CPU_INTO_SLEEP(m)  while(1)      //halSleep(m)   /* interface to HAL sleep */

///* used by MTEL.c */
//uint8_t OnBoard_SendKeys( uint8_t keys, uint8_t state );
//
///*
// * Board specific random number generator
// */
//extern uint16_t Onboard_rand( void );
//
///*
// * Get elapsed timer clock counts
// *   reset: reset count register if TRUE
// */
//extern uint32_t TimerElapsed( void );
//
///*
// * Initialize the Peripherals
// *    level: 0=cold, 1=warm, 2=ready
// */
//extern void InitBoard( uint8_t level );
//
///*
// * Register for all key events
// */
//extern uint8_t RegisterForKeys( uint8_t task_id );
//
///* Keypad Control Functions */
//
///*
// * Send "Key Pressed" message to application
// */
//extern uint8_t OnBoard_SendKeys(  uint8_t keys, uint8_t shift);
//
///*
// * Callback routine to handle keys
// */
//extern void OnBoard_KeyCallback ( uint8_t keys, uint8_t state );
//
///*
// * Perform a soft reset - jump to 0x0
// */
//extern void Onboard_soft_reset( void );


/*********************************************************************
 */

#endif
