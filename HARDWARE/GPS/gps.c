#include <stm32f10x.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "sys.h"
#include "gps.h"
#include "oled.h"
#include "usart.h"
#include "jq8x00.h"

// 外部全局结构体，存储GPS解析数据与状态
extern _SaveData Save_Data;

// 时间、纬度、经度、南北纬、东西经缓存数组
unsigned char time[11]   = {0};
unsigned char latitude[11] = {0};
unsigned char longitude[12] = {0};
unsigned char N_S[2]     = {0};
unsigned char E_W[2]     = {0};

/**
 * @brief  错误记录函数
 * @param  num: 错误编号
 * @retval 无
 */
void errorLog(int num)
{
    (void)num;  // 消除未使用参数警告，预留错误处理接口
}

/**
 * @brief  解析GPS NMEA帧数据
 * @note   按逗号分割字符串，提取时间、定位状态、经纬度等字段
 * @retval 无
 */
void parseGpsBuffer(void)
{
    char *subString;        // 分段解析起始指针
    char *subStringNext;    // 分段解析结束指针
    char i = 0;

    // 收到完整GPS数据后开始解析
    if (Save_Data.isGetData)
    {
        Save_Data.isGetData = 0;    // 清除数据接收标志
        // 循环解析前7个字段
        for (i = 0; i <= 7; i++)
        {
            // 第一个字段：跳过帧头，找到第一个逗号
            if (i == 0)
            {
                if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)
                    errorLog(1);
            }
            else
            {
                subString++;        // 从当前逗号下一位开始截取
                if ((subStringNext = strstr(subString, ",")) != NULL)
                {
                    char usefullBuffer[2] = {0};  // 定位状态缓存

                    // 根据字段序号提取对应数据
                    switch (i)
                    {
                        case 1:  // UTC时间
                            memcpy(Save_Data.UTCTime, subString, subStringNext - subString);
                            break;
                        case 2:  // 定位有效状态(A有效/V无效)
                            memcpy(usefullBuffer, subString, subStringNext - subString);
                            break;
                        case 3:  // 纬度
                            memcpy(Save_Data.latitude, subString, subStringNext - subString);
                            break;
                        case 4:  // 南北纬标识 N/S
                            memcpy(Save_Data.N_S, subString, subStringNext - subString);
                            break;
                        case 5:  // 经度
                            memcpy(Save_Data.longitude, subString, subStringNext - subString);
                            break;
                        case 6:  // 东西经标识 E/W
                            memcpy(Save_Data.E_W, subString, subStringNext - subString);
                            break;
                        case 7:  // 地面速度
                            memcpy(Save_Data.speed, subString, subStringNext - subString);
                            break;
                        default:
                            break;
                    }

                    subString = subStringNext;          // 指针切换至下一段
                    Save_Data.isParseData = 1;          // 标记解析完成

                    // 判断GPS定位是否有效
                    if (usefullBuffer[0] == 'A')
                        Save_Data.isUsefull = 1;
                    else if (usefullBuffer[0] == 'V')
                        Save_Data.isUsefull = 0;
                }
                else    // 未找到下一个逗号，数据格式异常
                {
                    errorLog(2);
                }
            }
        }
    }
}

/**
 * @brief  读取GPS数据并在OLED屏幕显示
 * @note   UTC时间转换为北京时间，刷新经纬度、方位标识
 * @retval 无
 */
void printGpsBuffer(void)
{
    u8 i, x, y;

    // 解析完成后刷新显示
    if (Save_Data.isParseData)
    {
        Save_Data.isParseData = 0;  // 清除解析标志，避免重复刷新

        // 拷贝原始UTC时间
        for (i = 0; i < 6; i++)
            time[i] = Save_Data.UTCTime[i];

        // UTC+8 转换为北京时间
        time[6] = (time[0] - '0') * 10 + (time[1] - '0') + 8;
        if (time[6] >= 24) time[6] -= 24;  // 24小时制溢出处理

        // 显示时间分隔符 :
        OLED_ShowString((45 + 8 * 2), 2, (u8*)":", 16, 0);
        OLED_ShowString((45 + 8 * 5), 2, (u8*)":", 16, 0);

        // 定位有效时，显示经纬度与方位
        if (Save_Data.isUsefull)
        {
            // 拷贝并显示纬度
            for (x = 0; x < 8; x++)
                latitude[x] = Save_Data.latitude[x];
            OLED_ShowChar((45 + 8 * 2), 6, latitude[4], 16, 0);
            N_S[0] = Save_Data.N_S[0];
            OLED_ShowChar(119, 6, N_S[0], 16, 0);

            // 拷贝并显示经度
            for (y = 0; y < 9; y++)
                longitude[y] = Save_Data.longitude[y];
            OLED_ShowChar((45 + 8 * 3), 4, longitude[5], 16, 0);
            E_W[0] = Save_Data.E_W[0];
            OLED_ShowChar(119, 4, E_W[0], 16, 0);
        }
    }
}

// 站点信息表：ID、纬度、经度、到站判断半径(m)
Station stations[STATION_NUM] =
{
	{0, 28.068777f, 113.001439f, 20},
	{1, 28.068630f, 113.000306f, 20},
	{2, 28.067965f, 113.000552f, 20},
	{3, 28.068303f, 113.001536f, 20}
};

/**
 * @brief  计算两点间地面直线距离(米)
 * @param  lat1,lng1: 第一点经纬度
 * @param  lat2,lng2: 第二点经纬度
 * @retval 两点距离(单位: m)
 */
float Get_Distance(float lat1, float lng1, float lat2, float lng2)
{
    float dx = (lng1 - lng2) * 111320 * cos(lat1 * 3.1415926f / 180.0f);
    float dy = (lat1 - lat2) * 111320;
    return sqrt(dx * dx + dy * dy);
}

/* NMEA ddmm.mmmm → 十进制度。例:"2804.1266" → 28.0687767 */
double NmeaToDeg(const char *raw)
{
    double v   = atof(raw);
    double deg = (double)((long)(v / 100.0));      /* 整数度:28      */
    double min = v - deg * 100.0;                  /* 余下的分:4.1266 */
    return deg + min / 60.0;                       /* 28.0687767     */
}

/**
 * @brief  判断当前车辆所处站点
 * @retval 站点ID(0~3) / 0xFF 未匹配到站点
 */
u8 Check_Current_Station(void)
{
    u8 i;
    float now_lat, now_lng;
    float dist;

    // GPS定位无效直接返回
    if (!Save_Data.isUsefull)
        return 0xFF;

    // 字符串经纬度转为浮点型
    now_lat = (float)NmeaToDeg((char*)Save_Data.latitude);
    now_lng = (float)NmeaToDeg((char*)Save_Data.longitude);
    if (Save_Data.N_S[0] == 'S') now_lat = -now_lat;   /* 南纬为负 */
    if (Save_Data.E_W[0] == 'W') now_lng = -now_lng;   /* 西经为负 */

    // 遍历所有站点，判断是否进入站点半径范围
    for (i = 0; i < STATION_NUM; i++)
    {
        dist = Get_Distance(now_lat, now_lng, stations[i].lat, stations[i].lng);
        if (dist < stations[i].radius)
        {
            return stations[i].id;
        }
    }
    return 0xFF;
}

/**
 * @brief  站点逻辑处理、语音播报、状态跳转、按键响应
 * @param  bus: 公交车状态结构体指针
 * @param  key: 按键键值
 * @retval 无
 */
void Station_Process(Bus *bus, u8 key)
{
    // OLED显示当前站、下一站信息
    OLED_Show_ADDRESS(bus->cur_sta, bus->next_sta);

    static u8 last_key = 0;
    // 语音芯片空闲时，显示到站提示
    if (JQ8x00_BUSY_Read && last_key != 3)
    {
        OLED_Show_NOTICE(bus->next_sta);
    }
    else
    {
        // 清空提示区域显存
        u8 i, n;
        for (i = 4; i < 6; i++)
            for (n = 16; n < 112; n++)
                OLED_GRAM[n][i] = 0;
    }

    // ==================== GPS自动到站播报逻辑 ====================
    static u8 gps_confirm_cnt = 0;   // 位置稳定计数(防抖)
    static u8 gps_triggered = 0;     // GPS播报触发标志

    if (!JQ8x00_BUSY_Read)
    {
        u8 detected = Check_Current_Station();
        
				// 检测到目标下一站
        if (detected == bus->next_sta && detected != 0xFF)
        {
            if (gps_confirm_cnt < 3)
            {
                gps_confirm_cnt++;
                // 连续3次检测到同一站点，判定到站并播报
                if (gps_confirm_cnt >= 3)
                {
                    JQ8x00_Command_Data(AppointTrack, 2);
                    JQ8x00_Command_Data(AppointTrack, 3 + bus->next_sta * 2);
                    gps_triggered = 1;
                }
            }
        }
        else
        {
            gps_confirm_cnt = 0; // 偏离站点，清零计数
        }
    }

    // ==================== 按键手动控制逻辑 ====================
    if (key == 2)   // 手动预播报/提示音
    {
        JQ8x00_Command_Data(AppointTrack, 4 + bus->next_sta * 2);
        last_key = 2;
    }
    else if (key == 3)  // 手动确认到站，切换站点
    {
        gps_confirm_cnt = 0;
        // 当前站更新为下一站
        bus->cur_sta = bus->next_sta;

        // 根据行驶方向更新下一站
        if (bus->orien_flag == 1)
        {
            bus->next_sta = bus->cur_sta + 1;
            if (bus->cur_sta == 3) { bus->orien_flag = -1; bus->next_sta = 2; }
        }
        else
        {
            bus->next_sta = bus->cur_sta - 1;
            if (bus->cur_sta == 0) { bus->orien_flag = 1; bus->next_sta = 1; }
        }
        OLED_Clear();
        // 播放新下一站语音
        JQ8x00_Command_Data(AppointTrack, 2);
        JQ8x00_Command_Data(AppointTrack, 3 + bus->next_sta * 2);
        last_key = 3;
    }

    // ==================== 语音播放结束后自动切换站点 ====================
    static u8 last_play = 0;
    u8 now_play = JQ8x00_BUSY_Read;
    // 检测到语音播放结束，且为GPS自动播报触发
    if (last_play == 1 && now_play == 0 && gps_triggered)
    {
        gps_confirm_cnt = 0;
        gps_triggered = 0;
        last_key = 0;

        // 更新当前站与下一站
        bus->cur_sta = bus->next_sta;
        if (bus->orien_flag == 1)
        {
            bus->next_sta = bus->cur_sta + 1;
            if (bus->cur_sta == 3)
            {
                bus->orien_flag = -1;
                bus->next_sta = 2;
            }
        }
        else
        {
            bus->next_sta = bus->cur_sta - 1;
            if (bus->cur_sta == 0)
            {
                bus->orien_flag = 1;
                bus->next_sta = 1;
            }
        }
        OLED_Clear();
    }
    last_play = now_play;
}
