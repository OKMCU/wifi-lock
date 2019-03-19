/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 3 $
 * $Date: 14/01/28 11:45a $
 * @brief    Template project for M051 series MCU
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
//#include "app\tasksched.h"
//#include "app\syseventsrv.h"
//#include "app\cmd.h"
//#include "app\tpsrv.h"

#include "hal_drivers.h"
#include "hal_mcu.h"
#include "hal_key.h"
#include "hal_atcmdsio.h"
#include "osal.h"
#include "M051Series.h"

#define 	SYSTICK_FREQ		1000
#define     DELAY_POR           0x002FFFFF
#define     DELAY_FWUPD         0x000FFFFF
static void SYS_PwrOnDelay(void)
{
    volatile uint32_t u32tmp;
    volatile uint32_t keyCnt;
    u32tmp = DELAY_POR;
    keyCnt = 0;
    HalATCmdSIODisable();
    while(u32tmp-- > 0)
    {
        if(HAL_STATE_KEY0() == HAL_KEY_STATE_PRESSED)
            keyCnt++;
    }

    if(keyCnt == DELAY_POR)
    {
        HalATCmdSIOFirmwareUpdateEnable();
        u32tmp = DELAY_FWUPD;
        while(u32tmp-- > 0);
        HalATCmdSIOEnable();
        while(1);
    }
}

static void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
	
	/*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    CLK_EnableXtalRC(CLK_PWRCON_OSC22M_EN_Msk);
	CLK_WaitClockReady(CLK_CLKSTATUS_OSC22M_STB_Msk);
	
	CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));
	
	//CLK_EnableXtalRC(CLK_PWRCON_XTL12M_EN_Msk);
    //CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk);

    CLK_SetCoreClock(50000000);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and cyclesPerUs automatically. */
    SystemCoreClockUpdate();
    
    /* Enable UART0 */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Set UART0 clock */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_PLL, CLK_CLKDIV_UART(1));
    /* Set P3 multi-function pins for UART0 RXD and TXD  */
	SYS->P3_MFP &= ~(SYS_MFP_P30_Msk | SYS_MFP_P31_Msk);
    SYS->P3_MFP |= (SYS_MFP_P30_RXD0 | SYS_MFP_P31_TXD0);

    /* Enable UART1 */
    CLK_EnableModuleClock(UART1_MODULE);
    /* Set UART1 clock */
    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART_S_PLL, CLK_CLKDIV_UART(1));
    /* Set P1 multi-function pins for UART1 RXD and TXD  */
	SYS->P1_MFP &= ~(SYS_MFP_P12_Msk | SYS_MFP_P13_Msk);
    SYS->P1_MFP |= (SYS_MFP_P12_RXD1 | SYS_MFP_P13_TXD1);

    CLK_EnableModuleClock(TMR0_MODULE);
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0_S_HCLK, 0);
    
    /*  Enable PWM01 */
    CLK_EnableModuleClock(PWM01_MODULE);
    /*  Set PWM01 clock */
    CLK_SetModuleClock(PWM01_MODULE, CLK_CLKSEL1_PWM01_S_HIRC, 0);
    //PWM_ConfigOutputChannel(PWMA, PWM_CH0, 2000, 50);
    //PWM_EnableOutput(PWMA, 0x1);
    //PWM_Start(PWMA, 0x1);
    /* Set P2 multi-function pins for PWM0 and PWM1  */
    SYS->P2_MFP &= ~(SYS_MFP_P20_PWM0);
    SYS->P2_MFP |= (SYS_MFP_P20_PWM0);

    /* Lock protected registers */
    SYS_LockReg();

    /* Start systick timer */
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, SYSTICK_FREQ);
    TIMER_EnableInt(TIMER0);
	NVIC_EnableIRQ(TMR0_IRQn);
	TIMER_Start(TIMER0);
    
}


int main( void )
{
    SYS_PwrOnDelay();
    HAL_DISABLE_INTERRUPTS();
    SYS_Init();
    /* Initialize hardware */
    // Initialize board I/O
    /* Initialze the HAL driver */
    HalDriverInit();

    /* Initialize NV system */
    /* Initialize the operating system */
    osal_init_system();

    /* Enable interrupts */
    HAL_ENABLE_INTERRUPTS();

    // Final board initialization
    //InitBoard( OB_READY );

#if defined ( POWER_SAVING )
    osal_pwrmgr_device( PWRMGR_BATTERY );
#endif

    /* Start OSAL */
    osal_start_system(); // No Return from here

    return 0;
}

/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/
