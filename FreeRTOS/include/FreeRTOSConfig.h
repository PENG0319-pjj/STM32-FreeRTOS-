#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ========== FreeRTOSConfig.h  适配 STM32F103C8T6 (Cortex-M3 @72MHz) ========== */

/* ---------- 基础调度 ---------- */
#define configUSE_PREEMPTION                    1     /* 抢占式调度 */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 72000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )   /* 1ms 一个节拍 */
#define configMAX_PRIORITIES                    ( 5 )                     /* 优先级 0~4 */
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 )/* 单位:字(4字节) */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 10 * 1024 ) ) /* 内核堆 10KB */
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0     /* CM3 用 32 位节拍 */
#define configIDLE_SHOULD_YIELD                 1

/* ---------- 同步机制 ---------- */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8

/* ---------- 软件定时器 ---------- */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( 2 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* ---------- 按需打开的内核 API ---------- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTaskResumeFromISR              1

/* ---------- 断言 / 检查(先关,第7步再打开) ---------- */
#define configASSERT( x )  if( ( x ) == 0 ) { portDISABLE_INTERRUPTS(); for( ;; ); }
#define configCHECK_FOR_STACK_OVERFLOW          2	//栈溢出检查（原0）
#define configUSE_MALLOC_FAILED_HOOK            0

/*
抢占优先级 0 ~ 4  ：最高最急,内核屏蔽不了它 → 【绝对不能】调用任何 FreeRTOS API
抢占优先级 5 ~ 15 ：内核临界区能屏蔽它     → 【可以】调用 ...FromISR() 系列 API
*/

/* ---------- Cortex-M3 中断优先级(关键,别改错) ---------- */
#define configPRIO_BITS                                 4    /* STM32 只用高 4 位 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* ---------- CM3 移植层:让内核接管这三个中断(关键) ---------- */
#define xPortPendSVHandler                      PendSV_Handler
#define vPortSVCHandler                         SVC_Handler
#define xPortSysTickHandler                     SysTick_Handler

#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TICKLESS_IDLE                 0

#endif /* FREERTOS_CONFIG_H */
