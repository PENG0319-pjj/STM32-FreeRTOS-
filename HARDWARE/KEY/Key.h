#ifndef __KEY_H__
#define __KEY_H__

#include "stm32f10x.h"                  // Device header
#include "delay.h"  
#include "ds1302.h"

extern struct TIMEData TimeData;


void Key_Init(void);
uint8_t Key_Read(void);
void time_deal_add(u8 time_index ,u8 value);
void time_deal_del(u8 time_index, u8 value);

void Key_Process(u8 key,u8 *mode,u8 *time_index);
#endif
