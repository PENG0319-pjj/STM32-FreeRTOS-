#include "stm32f10x.h"                  // Device header
#include "usart.h"
#include "sys.h"
#include <string.h>
#include <math.h>  // 需要在文件开头添加
#include <stdlib.h> // 添加 atof 函数支持
//============================ 移植过来的 GPS 全局变量 ============================
char rxdatabufer;
u16 point1 = 0;

_SaveData Save_Data;

char USART_RX_BUF[USART_REC_LEN];     // 接收缓冲
u16 USART_RX_STA=0;                   // 接收状态标记


//============================ 串口1 → JQ8900 ============================
void uart1_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);

    // TX PA9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // RX PA10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 中断配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=5;//(原3，适配FreeRTOS)
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//(原3，适配FreeRTOS)
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 串口参数
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

//============================ 串口2 → GPS（已移植GPS接收代码） ============================
void uart2_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // TX PA2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // RX PA3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 中断配置
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;//(原3，适配FreeRTOS)
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//(原3，适配FreeRTOS)
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 串口参数
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART2, &USART_InitStructure);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);

    CLR_Buf();      // 清空缓存
}

//============================ 串口1发送（JQ8900） ============================
void UART1_SendCode(u8 *DATA,u8 len)
{
    USART_ClearFlag(USART1,USART_FLAG_TC);
    while(len--)
    {
        USART_SendData(USART1, *DATA++);
        while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
    }
}

//============================ 串口2发送 ============================
void UART2_SendCode(u8 *DATA,u8 len)
{
    USART_ClearFlag(USART2,USART_FLAG_TC);
    while(len--)
    {
        USART_SendData(USART2, *DATA++);
        while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
    }
}

//============================ 串口1中断（JQ8900） ============================
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        USART_ReceiveData(USART1);  // 必须读取DR以清除RXNE中断标志
    }
}

//============================ 串口2中断 → 移植了你完整的GPS代码 ============================
//============================ 串口2中断 → GPS 修复版（只收不发 + 转发TTL） ============================
void USART2_IRQHandler(void)
{
    u8 Res;

    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        Res = USART_ReceiveData(USART2);   // 读取GPS数据

        // ===================== 关键 =====================
        // 从 PA2（串口2 TX）发给电脑！！！
        // ===============================================
        if (USART_GetFlagStatus(USART2, USART_FLAG_TXE) != RESET)
            USART_SendData(USART2, Res);

        // 下面是你原来的GPS接收代码，我完全保留！
        if(Res == '$')
        {
            point1 = 0;
        }

        if(point1 < USART_REC_LEN)  // 防溢出
        {
            USART_RX_BUF[point1++] = Res;
        }

        if(USART_RX_BUF[0] == '$' && USART_RX_BUF[4] == 'M' && USART_RX_BUF[5] == 'C')
        {
            if(Res == '\n')
            {
                memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);
                memcpy(Save_Data.GPS_Buffer, USART_RX_BUF, point1);
                Save_Data.isGetData = true;
                point1 = 0;
                memset(USART_RX_BUF, 0, USART_REC_LEN);
            }
        }
    }
}
//============================ 以下是你原来的工具函数（完整移植） ============================
u8 Hand(char *a)
{
    if(strstr(USART_RX_BUF,a)!=NULL)
        return 1;
    else
        return 0;
}

void CLR_Buf(void)
{
    memset(USART_RX_BUF, 0, USART_REC_LEN);
    point1 = 0;
}

void clrStruct(void)
{
    Save_Data.isGetData = false;
    Save_Data.isParseData = false;
    Save_Data.isUsefull = false;
    memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);
    memset(Save_Data.UTCTime, 0, UTCTime_Length);
    memset(Save_Data.latitude, 0, latitude_Length);
    memset(Save_Data.N_S, 0, N_S_Length);
    memset(Save_Data.longitude, 0, longitude_Length);
    memset(Save_Data.E_W, 0, E_W_Length);
}

//============================ 串口3 → ESP8266(115200) ============================
char USART3_RX_BUF[USART3_REC_LEN];
volatile u16 USART3_RX_CNT = 0;
volatile u8  USART3_RX_DONE = 0;

void uart3_init(u32 bound)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    /* USART3 挂在 APB1,而 GPIOB 挂在 APB2 —— 两个时钟都要开 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* TX PB10 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* RX PB11 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 中断:抢占优先级必须 >= 5 (内核临界区要能屏蔽它),和 USART1/2 一致 */
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART3, ENABLE);

    USART3_CLR_Buf();
}

void UART3_SendCode(u8 *DATA, u8 len)
{
    USART_ClearFlag(USART3, USART_FLAG_TC);
    while (len--)
    {
        USART_SendData(USART3, *DATA++);
        while (USART_GetFlagStatus(USART3, USART_FLAG_TC) != SET);
    }
}

void USART3_CLR_Buf(void)
{
    memset(USART3_RX_BUF, 0, USART3_REC_LEN);
    USART3_RX_CNT  = 0;
    USART3_RX_DONE = 0;
}

/* 收 ESP8266 的下行帧($ACK,... / $REQ*CS) —— 按行收,遇到 \n 置完成标志 */
void USART3_IRQHandler(void)
{
    u8 Res;

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        Res = USART_ReceiveData(USART3);       /* 必须读 DR 清标志 */

        if (Res == '$')                        /* 帧头,从头开始 */
            USART3_RX_CNT = 0;

        if (USART3_RX_CNT < USART3_REC_LEN - 1)
            USART3_RX_BUF[USART3_RX_CNT++] = Res;

        if (Res == '\n')
            USART3_RX_DONE = 1;
    }
}

