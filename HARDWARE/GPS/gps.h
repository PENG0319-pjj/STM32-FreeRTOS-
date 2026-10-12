#ifndef __GPS_H
#define __GPS_H

#include "stm32f10x.h"

// 站点结构体
typedef struct 
{
    u8 id;         // 0蒙德 1璃月 2稻妻 3至冬
    float lat;     // 纬度
    float lng;     // 经度
    u16 radius;    // 判定半径（米）
} Station;

typedef struct
{
	u8 cur_sta;
	u8 next_sta;
	s8 orien_flag;
}Bus;



// 外部声明
#define STATION_NUM  4   // 固定4个站
extern Station stations[STATION_NUM];
extern Bus bus;
// 函数声明
void parseGpsBuffer(void);
void printGpsBuffer(void);
u8 Check_Current_Station(void);  // 到站判断函数
double NmeaToDeg(const char *raw);   /* NMEA ddmm.mmmm → 十进制度 */
void Station_Process(Bus *bus,u8 key);
#endif
