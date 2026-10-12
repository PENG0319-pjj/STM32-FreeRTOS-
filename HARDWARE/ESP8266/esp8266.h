#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"

/* 上报帧:$DP,<lat>,<lng>,<fix>,<yy>,<mo>,<dd>,<hh>,<mm>,<ss>,<spd>*<CS>\r\n */
#define ESP_FRAME_MAX  96

u8     ESP_XorCheck(const char *p, u8 len);
u8     ESP_BuildFrame(char *buf);        /* 组帧,返回帧长 */

#endif
