#include "key.h"                 
#include "ds1302.h"
#include "oled.h"
#include "dht11.h"

void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8| GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed =GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}



void time_deal_add(u8 time_index ,u8 value)
{
	DS1302_write_rig(0x8e,0x00);//关闭写保护
	switch(time_index)
	{	
		case 1: // 年 2000~2099
			TimeData.year = (TimeData.year + value + 100) % 100 + 2000;
			DS1302_write_rig(0x8C, ((TimeData.year % 100 / 10) << 4) | (TimeData.year % 10));
		break;
        
		case 2: // 月 1~12
			TimeData.month = (TimeData.month - 1 + value + 12) % 12 + 1;
			DS1302_write_rig(0x88,((TimeData.month / 10) << 4) | (TimeData.month % 10));
		break;
        
		case 3: // 日 1~31（按月份自动判断）
			TimeData.day += value;
			if(TimeData.month == 1 || TimeData.month == 3 ||TimeData.month == 5 ||TimeData.month == 7 ||TimeData.month == 8
				 ||TimeData.month == 10  ||TimeData.month == 12)
			{
				if(TimeData.day > 31) TimeData.day = 1;
			}
			else if(TimeData.month == 4 || TimeData.month == 6 ||TimeData.month == 9 ||TimeData.month == 11)
			{
				if(TimeData.day > 30) TimeData.day = 1;
			}
			else
			{	
				if((TimeData.year%4==0 && TimeData.year%100!=0 ) || TimeData.year%400 == 0)
				{
					if(TimeData.day > 29) TimeData.day = 1;
				}
				else
				{
					if(TimeData.day > 28) TimeData.day = 1;
				}
			}
			DS1302_write_rig(0x86,((TimeData.day / 10) << 4) | (TimeData.day % 10));
		break;
        
		case 4: // 星期 1~7
			TimeData.week = (TimeData.week - 1 + value + 7) % 7 + 1;
			DS1302_write_rig(0x8a,TimeData.week);
		break;
        
		case 5: // 时 0~23
			TimeData.hour = (TimeData.hour + value) % 24;
			DS1302_write_rig(0x84,((TimeData.hour / 10) << 4) | (TimeData.hour % 10));
		break;
        
		case 6: // 分 0~59
			TimeData.minute = (TimeData.minute + value) % 60;
			DS1302_write_rig(0x82,((TimeData.minute / 10) << 4) | (TimeData.minute % 10));
		break;
        
		case 7: // 秒 0~59 + 保留暂停位
			TimeData.second = (TimeData.second + value) % 60;
			{
				u8 sec = ((TimeData.second/10)<<4) | (TimeData.second%10);
				u8 stop_bit = DS1302_read_rig(0x81) & 0x80;
				DS1302_write_rig(0x80, sec | stop_bit);
			}
		break;
	}
	DS1302_write_rig(0x8e,0x80);//开启写保护
}

void time_deal_del(u8 time_index, u8 value) // 统一加 value 参数
{
	DS1302_write_rig(0x8e,0x00);//关闭写保护
	switch(time_index)
	{	
		case 1: // 年 2000~2099
			TimeData.year = (TimeData.year - value + 100) % 100 + 2000;
			DS1302_write_rig(0x8C, ((TimeData.year % 100 / 10) << 4) | (TimeData.year % 10));
		break;

		case 2: // 月 1~12
			TimeData.month = (TimeData.month - 1 - value + 12) % 12 + 1;
			DS1302_write_rig(0x88,((TimeData.month / 10) << 4) | (TimeData.month % 10));
		break;

		case 3: // 日 1~31
			TimeData.day -= value;
			if(TimeData.day < 1)
			{
				if(TimeData.month == 1 || TimeData.month == 3 ||TimeData.month == 5 ||TimeData.month == 7 ||TimeData.month == 8
				 ||TimeData.month == 10  ||TimeData.month == 12)
				{
					TimeData.day = 31;
				}
				else if(TimeData.month == 4 || TimeData.month == 6 ||TimeData.month == 9 ||TimeData.month == 11)
				{
					TimeData.day = 30;
				}
				else
				{	
					if((TimeData.year%4==0 && TimeData.year%100!=0 ) || TimeData.year%400 == 0)
					{
						TimeData.day = 29;
					}
					else
					{
						TimeData.day = 28;
					}
				}
			}
			DS1302_write_rig(0x86,((TimeData.day / 10) << 4) | (TimeData.day % 10));
		break;

		case 4: // 星期 1~7
			TimeData.week = (TimeData.week - 1 - value + 7) % 7 + 1;
			DS1302_write_rig(0x8a,TimeData.week);
		break;

		case 5: // 时 0~23
			TimeData.hour = (TimeData.hour - value + 24) % 24;
			DS1302_write_rig(0x84,((TimeData.hour / 10) << 4) | (TimeData.hour % 10));
		break;

		case 6: // 分 0~59
			TimeData.minute = (TimeData.minute - value + 60) % 60;
			DS1302_write_rig(0x82,((TimeData.minute / 10) << 4) | (TimeData.minute % 10));
		break;

		case 7: // 秒 0~59 + 保留暂停位
			TimeData.second = (TimeData.second - value + 60) % 60;
			{
				u8 sec = ((TimeData.second/10)<<4) | (TimeData.second%10);
				u8 stop_bit = DS1302_read_rig(0x81) & 0x80;
				DS1302_write_rig(0x80, sec | stop_bit);
			}
		break;
	}
	DS1302_write_rig(0x8e,0x80);//开启写保护
}

uint8_t Key_Read(void)
{
	static uint16_t key_tick = 0;
	static uint8_t key_buf = 0;
	static uint8_t long_repeat_flag = 0;

	uint8_t cur_key = 0;

	// 扫描按键
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9)==0) cur_key = 1;
	else if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8)==0) cur_key = 2;
	else if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==0) cur_key = 3;
	else if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6)==0) cur_key = 4;
	else if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5)==0) cur_key = 5;

	// 松手复位
	if(cur_key == 0)
	{
		key_tick = 0;
		key_buf = 0;
		long_repeat_flag = 0;
		return 0;
	}

	// 第一次按下
	if(key_buf != cur_key)
	{
		key_buf = cur_key;
		key_tick = 0;
		long_repeat_flag = 0;
		delay_ms(20);
		return cur_key;
	}

	key_tick++;

	// 长按第一次触发：350ms
	if(key_tick >= 35)
	{
		long_repeat_flag = 1;
	}

	// 已经触发过长按，每隔 250ms 再发一次 +5 / -5
	if(long_repeat_flag)
	{
		if(key_tick >= 35 + 25)  // 连发间隔：250ms
		{
			key_tick = 35;      // 重置计时，循环触发
			if(cur_key == 3) return 6;
			if(cur_key == 4) return 7;
		}
	}

	return 0;
}

/*
	Key_Process:按键处理函数
	@key；按下的按键值
	@mode：当前模式的指针地址，方便在函数中修改
	@time_index：修改光标，理由同上
	返回值：
		void
*/
void Key_Process(u8 key,u8 *mode,u8 *time_index)
{
    switch(key)
    {
        case 1:  // 重置
            DS1302_Init();
            DHT11_Init();
            break;
            
        case 2:  // 切换光标
            if(*mode == 2)
            {
                if(++*time_index > 7)
                    *time_index = 1;
            }
            break;
            
        case 3:  // 短按 +1
            if(*mode == 2)
                time_deal_add(*time_index, 1);
            break;
            
        case 4:  // 短按 -1
            if(*mode == 2)
                time_deal_del(*time_index, 1);
            break;
            
        case 6:  // 长按 +5
            if(*mode == 2)
                time_deal_add(*time_index, 5);
            break;
            
        case 7:  // 长按 -5
            if(*mode == 2)
                time_deal_del(*time_index, 5);
            break;
            
        case 5:  // 切换模式
            OLED_Clear();
            if(++*mode > 2) 
            {
                *mode = 0;
                *time_index = 0;
                DS1302_Start();
            }
            if(*mode == 2)
            {
                DS1302_Pause();
            }
            break; 
    }
}
