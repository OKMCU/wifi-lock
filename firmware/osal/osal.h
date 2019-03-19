/******************************************************************************

 @file  osal.h

 @brief This API allows the software components in the Z-Stack to be written
        independently of the specifics of the operating system, kernel, or
        tasking environment (including control loops or connect-to-interrupt
        systems).

 Group: WCS, BTS
 Target Device: CC2540, CC2541

 ******************************************************************************
 
 Copyright (c) 2004-2016, Texas Instruments Incorporated
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
 PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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

#ifndef OSAL_H
#define OSAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

#include <limits.h>

#include "comdef.h"
#include "osal_memory.h"
#include "osal_timers.h"


/*********************************************************************
 * MACROS
 */
#if ( UINT_MAX == 65535 ) /* 8-bit and 16-bit devices */
  #define osal_offsetof(type, member) ((uint16_t) &(((type *) 0)->member))
#else /* 32-bit devices */
  #define osal_offsetof(type, member) ((uint32_t) &(((type *) 0)->member))
#endif

#define OSAL_MSG_NEXT(msg_ptr)      ((osal_msg_hdr_t *) (msg_ptr) - 1)->next

#define OSAL_MSG_Q_INIT(q_ptr)      *(q_ptr) = NULL

#define OSAL_MSG_Q_EMPTY(q_ptr)     (*(q_ptr) == NULL)

#define OSAL_MSG_Q_HEAD(q_ptr)      (*(q_ptr))

#define OSAL_MSG_LEN(msg_ptr)      ((osal_msg_hdr_t *) (msg_ptr) - 1)->len

#define OSAL_MSG_ID(msg_ptr)      ((osal_msg_hdr_t *) (msg_ptr) - 1)->dest_id

/*********************************************************************
 * CONSTANTS
 */

/*** Interrupts ***/
#define INTS_ALL    0xFF

/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  void   *next;
  uint16_t len;
  uint8_t  dest_id;
} osal_msg_hdr_t;

typedef struct
{
  uint8_t  event;
  uint8_t  status;
} osal_event_hdr_t;

typedef void * osal_msg_q_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * FUNCTIONS
 */

/*** Message Management ***/

  /*
   * Task Message Allocation
   */
  extern uint8_t * osal_msg_allocate(uint16_t len );

  /*
   * Task Message Deallocation
   */
  extern uint8_t osal_msg_deallocate( uint8_t *msg_ptr );

  /*
   * Send a Task Message
   */
  extern uint8_t osal_msg_send( uint8_t destination_task, uint8_t *msg_ptr );

  /*
   * Push a Task Message to head of queue
   */
  extern uint8_t osal_msg_push_front( uint8_t destination_task, uint8_t *msg_ptr );

  /*
   * Receive a Task Message
   */
  extern uint8_t *osal_msg_receive( uint8_t task_id );

  /*
   * Find in place a matching Task Message / Event.
   */
  extern osal_event_hdr_t *osal_msg_find(uint8_t task_id, uint8_t event);

  /*
   * Count the number of queued OSAL messages matching Task ID / Event.
   */
  extern uint8_t osal_msg_count(uint8_t task_id, uint8_t event);

  /*
   * Enqueue a Task Message
   */
  extern void osal_msg_enqueue( osal_msg_q_t *q_ptr, void *msg_ptr );

  /*
   * Enqueue a Task Message Up to Max
   */
  extern uint8_t osal_msg_enqueue_max( osal_msg_q_t *q_ptr, void *msg_ptr, uint8_t max );

  /*
   * Dequeue a Task Message
   */
  extern void *osal_msg_dequeue( osal_msg_q_t *q_ptr );

  /*
   * Push a Task Message to head of queue
   */
  extern void osal_msg_push( osal_msg_q_t *q_ptr, void *msg_ptr );

  /*
   * Extract and remove a Task Message from queue
   */
  extern void osal_msg_extract( osal_msg_q_t *q_ptr, void *msg_ptr, void *prev_ptr );


/*** Task Synchronization  ***/

  /*
   * Set a Task Event
   */
  extern uint8_t osal_set_event( uint8_t task_id, uint16_t event_flag );


  /*
   * Clear a Task Event
   */
  extern uint8_t osal_clear_event( uint8_t task_id, uint16_t event_flag );


/*** Interrupt Management  ***/

  /*
   * Register Interrupt Service Routine (ISR)
   */
  extern uint8_t osal_isr_register( uint8_t interrupt_id, void (*isr_ptr)( uint8_t* ) );

  /*
   * Enable Interrupt
   */
  extern uint8_t osal_int_enable( uint8_t interrupt_id );

  /*
   * Disable Interrupt
   */
  extern uint8_t osal_int_disable( uint8_t interrupt_id );


/*** Task Management  ***/

  /*
   * Initialize the Task System
   */
  extern uint8_t osal_init_system( void );

  /*
   * System Processing Loop
   */
  extern void osal_start_system( void );

  /*
   * One Pass Through the OSAL Processing Loop
   */
  extern void osal_run_system( void );

  /*
   * Get the active task ID
   */
  extern uint8_t osal_self( void );


/*** Helper Functions ***/

  /*
   * String Length
   */
  extern int osal_strlen( char *pString );

  /*
   * Memory copy
   */
  extern void *osal_memcpy( void*, const void GENERIC *, unsigned int );

  /*
   * Memory Duplicate - allocates and copies
   */
  extern void *osal_memdup( const void GENERIC *src, unsigned int len );

  /*
   * Reverse Memory copy
   */
  extern void *osal_revmemcpy( void*, const void GENERIC *, unsigned int );

  /*
   * Memory compare
   */
  extern uint8_t osal_memcmp( const void GENERIC *src1, const void GENERIC *src2, unsigned int len );

  /*
   * Memory set
   */
  extern void *osal_memset( void *dest, uint8_t value, int len );

  /*
   * Build a uint16_t out of 2 bytes (0 then 1).
   */
  extern uint16_t osal_build_uint16( uint8_t *swapped );

  /*
   * Build a uint32_t out of sequential bytes.
   */
  extern uint32_t osal_build_uint32( uint8_t *swapped, uint8_t len );

  /*
   * Convert long to ascii string
   */
    extern uint8_t *_ltoa( uint32_t l, uint8_t * buf, uint8_t radix );

  /*
   * Random number generator
   */
  extern uint16_t osal_rand( void );

  /*
   * Buffer an uint32_t value - LSB first.
   */
  extern uint8_t* osal_buffer_uint32( uint8_t *buf, uint32_t val );

  /*
   * Buffer an uint24 value - LSB first
   */
  extern uint8_t* osal_buffer_uint24( uint8_t *buf, uint24 val );

  /*
   * Is all of the array elements set to a value?
   */
  extern uint8_t osal_isbufset( uint8_t *buf, uint8_t val, uint8_t len );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OSAL_H */