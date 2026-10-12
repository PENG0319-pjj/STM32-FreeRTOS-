#include "stm32f10x.h"
#include "led.h"
#include "usart.h"
#include "delay.h"
#include "oled.h"
#include "ds1302.h"
#include "timer.h"
#include "key.h"
#include "dht11.h"
#include "jq8x00.h"
#include "gps.h"
#include "esp8266.h"
/* ---------- FreeRTOS ---------- */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// 外部变量
extern _SaveData Save_Data;
extern volatile u8 DDD;
volatile u8 DDD=0;
volatile u8 TTT=0;
u8 temp, humi;

u8 st[50];
u8 key = 0;       // 按键值
u8 mode = 0;      // 当前模式
u8 time_index =0; // 时间调整索引
u8 flag =0;
static u8 ttt_handled = 0;
u8 auto_switch = 0;
Bus bus={0,1,1};

/* ==================== 任务间通信 ==================== */
QueueHandle_t xKeyQueue = NULL;   /* 按键:Key_Task → UI_Task */

/* 栈溢出钩子:灯狂闪 = 有任务栈爆了(见 configCHECK_FOR_STACK_OVERFLOW) */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    volatile u32 i;
    (void)xTask; (void)pcTaskName;
    portDISABLE_INTERRUPTS();
    for (;;)
    {
        LED_Toggle();
        for (i = 0; i < 400000; i++);
    }
}

/* ==================== 任务 ==================== */

/* 按键任务:每 10ms 扫一次(原超循环节奏),键值投队列 */
void Key_Task(void *pvParameters)
{
    (void)pvParameters;
    u8 k;
    for (;;)
    {
        k = Key_Read();
        if (k != 0)
            xQueueSend(xKeyQueue, &k, 0);
        vTaskDelay(10);
    }
}

/* 界面任务:【独占 OLED】按键处理 + GPS 字段 + 页面 + 刷屏 */
void UI_Task(void *pvParameters)
{
    (void)pvParameters;
    u8 key = 0;

    for (;;)
    {
        /* 1. 从队列取按键(非阻塞) */
        if (xQueueReceive(xKeyQueue, &key, 0) != pdTRUE)
            key = 0;

        /* 2. 站台页按 4:切换自动翻页 */
        if (key == 4 && mode == 1)
        {
            auto_switch = !auto_switch;
            key = 0;
            LED_Twinkle();
        }

        /* 3. 按键处理 */
        Key_Process(key, &mode, &time_index);

        /* 4. GPS 字段:保持原顺序(先画 GPS,页面后画盖住它) */
        if (mode != 1) printGpsBuffer();

        /* 5. 5 秒自动翻页(原 TTT 逻辑) */
        if (TTT == 0 && !ttt_handled && auto_switch && mode != 2)
        {
            ttt_handled = 1;
            {
                u8 prev = mode;
                mode = (mode + 1) % 2;
                if (mode != prev)
                {
                    u8 i, n;
                    for (i = 0; i < 8; i++)
                        for (n = 0; n < 128; n++)
                            OLED_GRAM[n][i] = 0;
                }
            }
        }
        if (TTT != 0) ttt_handled = 0;

        /* 6. 画页面 + 刷屏 */
        switch (mode)
        {
            case 0:
                OLED_Show_DHT();
                OLED_Show_Time(st, time_index, mode);
                break;
            case 1:
                Station_Process(&bus, key);
                break;
            case 2:
                OLED_Show_Time(st, time_index, mode);
                break;
        }
        OLED_Refresh();

        vTaskDelay(10);
    }
}

/* GPS 任务:只解析数据,【不碰屏幕】 */
void GPS_Task(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        parseGpsBuffer();
        vTaskDelay(10);
    }
}

/* 心跳灯 */
void LED_Task(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        LED_Toggle();
        vTaskDelay(500);
    }
}

/* 上报任务:每 10 秒组一帧,从 USART3 发给 ESP8266 */
void Report_Task(void *pvParameters)
{
    (void)pvParameters;
    static char frame[ESP_FRAME_MAX];
    TickType_t  last = xTaskGetTickCount();
    u8 sec_cnt = 0;

    for (;;)
    {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));   /* 精确 1 秒 */
        if (++sec_cnt >= 10)
        {
            sec_cnt = 0;
            UART3_SendCode((u8*)frame, ESP_BuildFrame(frame));  /* 阻塞 ~5ms,但没人等它 */
        }
    }
}

int main(void)
{
	SystemInit();// 系统时钟初始化为72M

	delay_init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	TIM3_Int_Init(9999, 7199);
	uart1_init(9600);
	uart2_init(9600);
  uart3_init(115200);
	JQ8x00_Init();
	JQ8x00_Command_Data(SetVolume,30);
	DS1302_GPIO_Init();
	DS1302_Init();
	DS1302_read_realTime();
	clrStruct();
	LED_Init();
	delay_ms(3000);
	OLED_Init();
	Key_Init();
	OLED_Clear();
	DHT11_Init();
	JQ8x00_Command_Data(AppointTrack, 1);

	/* ---------- 创建 IPC 对象 ---------- */
	xKeyQueue = xQueueCreate(8, sizeof(u8));
	if (xKeyQueue == NULL)
	{
		while (1);   /* 堆不够,正常不会发生 */
	}

	/* ---------- 创建任务 ---------- */
	xTaskCreate(Key_Task, "KEY", 128, NULL, 3, NULL);
	xTaskCreate(UI_Task,  "UI",  256, NULL, 2, NULL);
	xTaskCreate(GPS_Task, "GPS", 256, NULL, 1, NULL);
	xTaskCreate(LED_Task, "LED", 128, NULL, 1, NULL);
    xTaskCreate(Report_Task, "RPT", 256, NULL, 1, NULL);

	/* ---------- 启动调度器(永不返回) ---------- */
	vTaskStartScheduler();

	while(1);
}

// TIM3中断服务函数 1秒执行一次
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        DDD++;        	// 标志置1，表示1秒到
		TTT=(TTT+1)%5;	//5秒定时器
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}
