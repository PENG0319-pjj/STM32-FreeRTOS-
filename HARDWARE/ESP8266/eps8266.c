#include "esp8266.h"
#include "usart.h"
#include "ds1302.h"
#include "gps.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern _SaveData Save_Data;     /* usart.h 里只有类型定义,变量在这里声明 */


/* ---- XOR 校验:对 p[0..len-1] 逐字节异或 ---- */
u8 ESP_XorCheck(const char *p, u8 len)
{
    u8 cs = 0;
    while (len--) cs ^= (u8)(*p++);
    return cs;
}

/* ---- 把"微度"整数写成 ddd.dddddd(正数不带 +,和协议示例一致) ---- */
static void put_deg(char *p, long udeg)
{
    unsigned long a;
    char sign = 0;
    if (udeg < 0) { sign = '-'; a = (unsigned long)(-udeg); }
    else          {              a = (unsigned long)udeg;   }

    if (sign) sprintf(p, "-%lu.%06lu", a / 1000000UL, a % 1000000UL);
    else      sprintf(p,  "%lu.%06lu", a / 1000000UL, a % 1000000UL);
}

/* ---- 组一帧 $DP,...*CS\r\n ---- */
u8 ESP_BuildFrame(char *buf)
{
    char   body[ESP_FRAME_MAX];
    char   lat_s[16], lng_s[16];
    double lat_d, lng_d;
    long   lat, lng, spd;
    u8     cs;

    /* 1. 经纬度:NMEA → 十进制度 → 微度整数(6 位小数) */
    lat_d = NmeaToDeg(Save_Data.latitude);
    lng_d = NmeaToDeg(Save_Data.longitude);
    if (Save_Data.N_S[0] == 'S') lat_d = -lat_d;   /* 南纬为负 */
    if (Save_Data.E_W[0] == 'W') lng_d = -lng_d;   /* 西经为负 */

    lat = (long)(lat_d * 1000000.0 + (lat_d >= 0 ? 0.5 : -0.5));
    lng = (long)(lng_d * 1000000.0 + (lng_d >= 0 ? 0.5 : -0.5));
    put_deg(lat_s, lat);
    put_deg(lng_s, lng);

    /* 2. 速度(GPS 的节数;无数据就是 0) */
    spd = (Save_Data.speed[0] != 0) ? (long)atof(Save_Data.speed) : 0L;

    /* 3. 组装 body($ 和 *CS 之间的部分)。时间戳来自 DS1302 */
    sprintf(body, "DP,%s,%s,%d,%02d,%02d,%02d,%02d,%02d,%02d,%ld",
            lat_s, lng_s,
            (Save_Data.isUsefull ? 1 : 0),
            TimeData.year % 100, TimeData.month, TimeData.day,
            TimeData.hour, TimeData.minute, TimeData.second,
            spd);

    /* 4. XOR 校验(覆盖 body 全部字节) */
    cs = ESP_XorCheck(body, (u8)strlen(body));

    /* 5. 拼成完整帧 */
    return (u8)sprintf(buf, "$%s*%02X\r\n", body, cs);
}
