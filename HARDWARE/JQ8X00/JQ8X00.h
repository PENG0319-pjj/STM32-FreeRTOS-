#ifndef __JQ8x00_H
#define __JQ8x00_H

#include "stm32f10x.h"

// 必须加上这两个，否则 Delay、UART 函数找不到
#include "delay.h"
#include "usart.h"

/**********************宏定义区**************************/
#define JQ8x00_Workmode		0		// 0=串口控制，1=单线控制
#define JQ8x00_BusyCheck	1		// 0=不检测忙，1=检测忙

#define ZH_MAX	50		//组合播放最大容量

//延时函数重定义
#define JQ8x00_ms(ms)    Delay_ms(ms)
#define JQ8x00_us(us)    Delay_us(us)

//串口发送函数重定义
#define JQ8x00_UART(pointer,len) UART1_SendCode(pointer,len)   // 这里改成你实际串口函数名



/*************************IO端口宏定义*********************************/
//忙检测IO
#define JQ8x00_BUSY_RCC          RCC_APB2Periph_GPIOA
#define JQ8x00_BUSY_GPIO         GPIOA
#define JQ8x00_BUSY_GPIO_Pin     GPIO_Pin_4

#define JQ8x00_BUSY_Read  GPIO_ReadInputDataBit(JQ8x00_BUSY_GPIO,JQ8x00_BUSY_GPIO_Pin)

//单线串口IO
#define JQ8x00_VPP_RCC           RCC_APB2Periph_GPIOA
#define JQ8x00_VPP_GPIO         GPIOA
#define JQ8x00_VPP_GPIO_Pin     GPIO_Pin_1

#define JQ8x00_VPP_Clr()	 GPIO_ResetBits(JQ8x00_VPP_GPIO,JQ8x00_VPP_GPIO_Pin)
#define JQ8x00_VPP_Set()	 GPIO_SetBits(JQ8x00_VPP_GPIO,JQ8x00_VPP_GPIO_Pin)

//循环模式
typedef enum {
    OverallCycle                = 0X00,
    SingleCycle                 = 0x01,
    SingleStop                  = 0x02,
    OverallRandom               = 0X03,
    CatalogCycle                = 0X04,
    CatalogRandom               = 0x05,
    CatalogTurnPlay             = 0x06,
    OverallTurnPlay             = 0x07,
}LoopModeSelect;

#if JQ8x00_Workmode
/**********************单线控制**************************/
typedef enum {
    Play 		= 0x11,
    Pause		= 0x12,
    Stop		= 0x13,
    LastSong    = 0x14,
    NextSong	= 0x15,
    LastList	= 0x16,
    NextList	= 0x17,
    ChooseSD	= 0x18,
    ChooseUdisk	= 0x19,
    ChooseFlash	= 0x1a,
    SysSleep	= 0x1b,
} LineByteSelect;

typedef enum {
    Track 		= 0x0B,
    Volume		= 0x0C,
    EQ			= 0x0D,
    LoopMode    = 0x0E,
    Channel		= 0x0F,
    CBTrack		= 0x10,
} LineModeSelect;

void OneLine_ByteControl(LineByteSelect Mode);
void OneLine_ZHControl(u8 Nume,LineModeSelect Mode);
void OneLine_SendData(u8 DATA);

#else
/**********************串口控制**************************/
typedef enum {
    CheckPlayState                  = 0x01,
    Play                            = 0x02,
    pause                           = 0x03,
    Stop                            = 0x04,
    LastSong                        = 0x05,
    NextSong                        = 0x06,
    CheckOnelineDisksign	        = 0x09,
    CheckCurrentDisksign	        = 0X0A,
    CheckTotalTrack                 = 0x0C,
    CurrentTrack                    = 0x0D,
    LastFloder                      = 0x0E,
    NextFloder                      = 0x0F,
    EndPlay	                        = 0x10,
    CheckFloderFirstTrack           = 0x11,
    CheckFloderAllTrack             = 0x12,
    AddVolume                       = 0x14,
    DecVolume                       = 0x15,
    EndZHPlay                       = 0X1C,
    CheckSongShortName	            = 0x1E,
    EndLoop                         = 0x21,
    GetTotalSongTime                = 0x24,
    OpenPlayTime                    = 0x25,
    ClosePlayTime                   = 0x26,
}UartCommand;

typedef enum {
    AppointTrack                    = 0x07,
    SetCycleCount                   = 0x19,
    SetEQ                           = 0X1A,
    SelectTrackNoPlay               = 0x19,
    GoToDisksign                    = 0X0B,
    SetVolume                       = 0x13,
    SetLoopMode                     = 0x18,
    SetChannel                      = 0x1D,
    AppointTimeBack                 = 0x22,
    AppointTimeFast                 = 0x23,
}UartCommandData;

typedef enum {
    JQ8X00_USB                      = 0X00,
    JQ8X00_SD                       = 0x01,
    JQ8X00_FLASH                    = 0x02,
}JQ8X00_Symbol;

void JQ8x00_Command(UartCommand MODE);
void JQ8x00_Command_Data(UartCommandData MODE,u8 DATA);
void JQ8x00_ZuHeBoFang(u8 *DATA,u8 Len);
void JQ8x00_RandomPathPlay(JQ8X00_Symbol MODE,char *DATA);
#endif

void JQ8x00_Init(void);

#endif
