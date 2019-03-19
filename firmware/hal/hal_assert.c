/******************************************************************************

 @file  hal_assert.c

 @brief Describe the purpose and contents of the file.

 Group: WCS, BTS
 Target Device: CC2540, CC2541

 ******************************************************************************
 
 Copyright (c) 2006-2016, Texas Instruments Incorporated
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


/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_assert.h"
#include "hal_types.h"
//#include "hal_board.h"
#include "hal_defs.h"
#include "hal_mcu.h"
#include "hal_led.h"
#include "hal_key.h"

/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void halAssertHazardLights(void);


/**************************************************************************************************
 * @fn          halAssertHandler
 *
 * @brief       Logic to handle an assert.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void halAssertHandler( void )
{
#if defined( HAL_ASSERT_RESET )
  HAL_SYSTEM_RESET();
#elif defined ( HAL_ASSERT_LIGHTS )
  halAssertHazardLights();
#elif defined( HAL_ASSERT_SPIN )
  volatile uint8_t i = 1;
  HAL_DISABLE_INTERRUPTS();
  while(i);
#endif

  return;
}

/**************************************************************************************************
 * @fn          halAssertHazardLights
 *
 * @brief       Blink LEDs to indicate an error.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void halAssertHazardLights(void)
{   
    HAL_DISABLE_INTERRUPTS();
    
    HAL_TURN_ON_LED0();
    HAL_TURN_ON_LED1();
    HAL_TURN_ON_LED2();
    //HAL_TURN_ON_LED3();

    for(;;)
    {
        if(HAL_STATE_KEY0() == HAL_KEY_STATE_PRESSED)
        {
            break;
        }
    }
}

/* ------------------------------------------------------------------------------------------------
 *                                    Compile Time Assertions
 * ------------------------------------------------------------------------------------------------
 */

/* integrity check of type sizes */
HAL_ASSERT_SIZE(  int8_t, 1);
HAL_ASSERT_SIZE( uint8_t, 1);
HAL_ASSERT_SIZE( int16_t, 2);
HAL_ASSERT_SIZE(uint16_t, 2);
HAL_ASSERT_SIZE( int32_t, 4);
HAL_ASSERT_SIZE(uint32_t, 4);


/**************************************************************************************************
*/
