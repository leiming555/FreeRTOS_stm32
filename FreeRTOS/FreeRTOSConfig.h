#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

//配置内核调度方式  1为抢占式  0为合作式(时间片)
#define configUSE_PREEMPTION		1
//配置是否使用IDLE空闲任务钩子函数
#define configUSE_IDLE_HOOK			0
//配置是否使用TICK滴答任务钩子函数
#define configUSE_TICK_HOOK			0
//配置CPU主频(HZ)
#define configCPU_CLOCK_HZ			( ( unsigned long ) 72000000 )	
//配置系统时钟节拍数(HZ)
#define configTICK_RATE_HZ			( ( TickType_t ) 1000 )
//配置供用户使用的最大优先级数
#define configMAX_PRIORITIES		( 5 )
//配置空闲任务的栈大小
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 128 )
//配置堆大小
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 10 * 1024 ) )
//配置任务名最大的字符数
#define configMAX_TASK_NAME_LEN		( 16 )
//配置是否使用可视化跟踪调试
#define configUSE_TRACE_FACILITY	0
/*  
系统时钟节拍计数使用TickType_t数据类型定义的。
如果用户使能了宏定义 configUSE_16_BIT_TICKS，
那么TickType_t定义的就是16位无符号数，
如果没有使能，那么TickType_t定义的就是32位无符号数。
对于32位架构的处理器，一定要禁止此宏定义，即设置此宏定义数值为0即可。
而16位无符号数类型主要用于8位和16位架构的处理器。
*/
#define configUSE_16_BIT_TICKS		0
/*
此参数用于使能与空闲任务同优先级的任务，只有满足以下两个条件时，此参数才有效果：
1.使能抢占式调度器。
2.有创建与空闲任务同优先级的任务。
配置为1，就可以使能此特性了，实际应用中不建议用户使用此功能，将其配置为0即可。
*/
#define configIDLE_SHOULD_YIELD		1

/***********************************************************/

//队列使用时要开启
#define configUSE_COUNTING_SEMAPHORES 	1

//软件定时器使用时要开启
#define configUSE_TIMERS                0
//软件定时器的优先级
#define configTIMER_TASK_PRIORITY       50
//软件定时器栈大小
#define configTIMER_TASK_STACK_DEPTH    50
//软件定时器队列大小
#define configTIMER_QUEUE_LENGTH        50

/***********************************************************/

/* Co-routine definitions. */
//配置是否使能合作式调度相关函数
#define configUSE_CO_ROUTINES 		0
//定义可供用户使用的最大的合作式任务优先级数
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

#define INCLUDE_vTaskPrioritySet        1       //设置任务优先级
#define INCLUDE_uxTaskPriorityGet        1        //获取任务优先级
#define INCLUDE_vTaskDelete                1        //删除任务
#define INCLUDE_vTaskCleanUpResources    0        //清理任务资源
#define INCLUDE_vTaskSuspend            1        //挂起任务
#define INCLUDE_vTaskDelayUntil            1        //延时直至
#define INCLUDE_vTaskDelay                1        //延时

//配置内核中断优先级数
#define configKERNEL_INTERRUPT_PRIORITY 		255

/*
低于此优先级的中断可以安全调用FreeRTOS的API函数，
高于此优先级的中断是FreeRTOS不能禁止的，
中断服务函数也不能调用FreeRTOS的API函数！
如以STM32为例，设置NVIC优先级分组为4，即有16个抢占优先级，0最高，15最低，
设置configMAX_SYSCALL_INTERRUPT_PRIORITY为5，则：
优先级0-4不会被FreeRTOS禁止，中断不可以调用FreeRTOS的API函数。
优先级5-15可以调用以FROM_ISR结尾的API函数，并且它们可以中断嵌套。
*/
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191

/*
这个值是将configMAX_SYSCALL_INTERRUPT_PRIORITY与运算，
取出芯片支持的优先级有效位所得到的值。
程序中未使用。只是作为用户定义抢占优先级的参考
*/
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

#define xPortPendSVHandler PendSV_Handler
#define vPortSVCHandler SVC_Handler
//#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
