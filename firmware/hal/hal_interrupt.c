#include "hal_interrupt.h"
#include "M051Series.h"
#include "hal_uart.h"
#include "osal_clock.h"

extern void UART0_IRQHandler(void)
{
    HalUartIsr(HAL_UART_PORT_0);
}

extern void UART1_IRQHandler(void)
{
    HalUartIsr(HAL_UART_PORT_1);
}

extern void TMR0_IRQHandler(void)
{
    if(TIMER_GetIntFlag(TIMER0) == 1)
    {
        /* Clear Timer0 time-out interrupt flag */
        TIMER_ClearIntFlag(TIMER0);
        osalSystickUpdate();
    }
}

