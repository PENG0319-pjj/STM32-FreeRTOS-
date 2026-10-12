#include "delay.h"

#define DEMCR_TRCENA   (1UL << 24)                       /* CoreDebug->DEMCR.TRCENA */
#define DWT_CTRL       (*(volatile u32 *)0xE0001000)     /* DWT 控制寄存器  */
#define DWT_CYCCNT     (*(volatile u32 *)0xE0001004)     /* DWT 周期计数    */

static u32 fac_us = 0;   /* 1us 需要多少个 CPU 周期 */

/* 不再使用 SysTick —— 它已经交给 FreeRTOS 做系统节拍了 */
void delay_init(u8 SYSCLK)
{
    fac_us = SYSCLK;                     /* 72MHz -> 72 周期/us */

    CoreDebug->DEMCR |= DEMCR_TRCENA;    /* 打开 DWT */
    DWT_CYCCNT = 0;                      /* 清计数   */
    DWT_CTRL  |= 1UL;                    /* CYCCNTENA: 开始计数 */
}

void delay_us(u32 nus)
{
    u32 start  = DWT_CYCCNT;
    u32 target = nus * fac_us;
    while ((DWT_CYCCNT - start) < target);   /* 32 位无符号回绕天然正确 */
}

void delay_ms(u16 nms)
{
    while (nms--) delay_us(1000);
}
