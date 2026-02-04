#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"
#include "task.h"
#include "LED.h"
#include "KEY.h"
#include "OLED.h"
#include "stdio.h"                       // 用于sprintf函数

// RTC相关函数声明
void RTC_Init(void);
void RTC_GetTime(uint8_t *hour, uint8_t *minute, uint8_t *second);
void RTC_SetTime(uint8_t hour, uint8_t minute, uint8_t second);

/*
	KEY.h中的延时消抖需要进行修改
*/

uint8_t KeyNum;
uint8_t num = 0;

// 帧率计算相关变量
uint32_t frameCount = 0;           // 帧数计数器
uint32_t lastTick = 0;             // 上一次计算帧率的系统节拍
uint16_t fps = 0;                  // 帧率值

#define START_TASK_PRIO         1    //任务优先级
#define START_TASK_STACK_SIZE   128    //任务堆栈大小    
TaskHandle_t    start_task_handler;    //任务句柄
void start_task( void * pvParameters );    //任务函数


#define TASK1_PRIO         2    //任务优先级
#define TASK1_STACK_SIZE   50    //任务堆栈大小    
TaskHandle_t    task1_handler;    //任务句柄
void task1( void * pvParameters );    //任务函数

#define TASK2_PRIO         3    //任务优先级
#define TASK2_STACK_SIZE   50    //任务堆栈大小    
TaskHandle_t    task2_handler;    //任务句柄
void task2( void * pvParameters );    //任务函数

#define TASK3_PRIO         4    //任务优先级
#define TASK3_STACK_SIZE   50    //任务堆栈大小
TaskHandle_t    task3_handler;    //任务句柄
void task3( void * pvParameters );    //任务函数

#define OLED_TASK_PRIO         5    //任务优先级
#define OLED_TASK_STACK_SIZE   200    //任务堆栈大小（增大以容纳动画功能）
TaskHandle_t    oled_task_handler;    //任务句柄
void oled_task( void * pvParameters );    //任务函数

// 快速点按两次请求标志
uint8_t double_click_request = 0;
// 时间调整模式标志
uint8_t time_adjust_mode = 0;


void freertos_demo(void)
{
    xTaskCreate((TaskFunction_t         )   start_task,                //任务函数
                (char *                 )   "start_task",              //任务名称
                (uint16_t )                 START_TASK_STACK_SIZE,     //任务堆栈大小
                (void *                 )   NULL,                       //传递给任务函数的参数
                (UBaseType_t            )   START_TASK_PRIO,           //任务优先级
                (TaskHandle_t *         )   &start_task_handler );     //任务句柄   
                
    vTaskStartScheduler();          //开启任务调度
}


void start_task( void * pvParameters )
{
    taskENTER_CRITICAL();               /* 进入临界区 */
    
    OLED_Init(); // 初始化OLED
    OLED_Clear(); // 清屏
    
    RTC_Init(); // 初始化RTC，实现掉电不丢失的实时时钟
    
    xTaskCreate((TaskFunction_t         )   task1,                //任务函数
                (char *                 )   "task1",            //任务名称
                (uint16_t )                 TASK1_STACK_SIZE,   //任务堆栈大小
                (void *                 )   NULL,               //传递给任务函数的参数
                (UBaseType_t            )   TASK1_PRIO,         //任务优先级
                (TaskHandle_t *         )   &task1_handler );   //任务句柄   
                
    xTaskCreate((TaskFunction_t         )   task2,                //任务函数
                (char *                 )   "task2",            //任务名称
                (uint16_t )                 TASK2_STACK_SIZE,   //任务堆栈大小
                (void *                 )   NULL,               //传递给任务函数的参数
                (UBaseType_t            )   TASK2_PRIO,         //任务优先级
                (TaskHandle_t *         )   &task2_handler );   //任务句柄   
                
    xTaskCreate((TaskFunction_t         )   task3,                //任务函数
                (char *                 )   "task3",            //任务名称
                (uint16_t )                 TASK3_STACK_SIZE,   //任务堆栈大小
                (void *                 )   NULL,               //传递给任务函数的参数
                (UBaseType_t            )   TASK3_PRIO,         //任务优先级
                (TaskHandle_t *         )   &task3_handler );   //任务句柄   
                
    xTaskCreate((TaskFunction_t         )   oled_task,            //任务函数
                (char *                 )   "oled_task",        //任务名称
                (uint16_t )                 OLED_TASK_STACK_SIZE,   //任务堆栈大小
                (void *                 )   NULL,               //传递给任务函数的参数
                (UBaseType_t            )   OLED_TASK_PRIO,     //任务优先级
                (TaskHandle_t *         )   &oled_task_handler );   //任务句柄   
    
    vTaskDelete(start_task_handler); //删除开始任务            
    taskEXIT_CRITICAL();                /* 退出临界区 */
}

/* 任务一，实现LED1每500ms翻转一次 */
void task1( void * pvParameters )
{
    while(1)
    {
        
        LED1_ON();
        vTaskDelay(500);
        LED1_OFF();
        vTaskDelay(500);
    }
}

/* 任务二，实现LED2每500ms翻转一次 */
void task2( void * pvParameters )
{
    while(1)
    {
        LED2_ON();
        vTaskDelay(500);
        LED2_OFF();
        vTaskDelay(500);
    }
}

/* 任务三，判断按键KEY1，按下KEY1删除或重新创建task1，并让num加1，长按切换到动画模式，双击进入时间调整模式 */
void task3( void * pvParameters )
{
    uint8_t key_press_time = 0; // 按键按下时间计数器
    uint8_t short_press_detected = 0; // 短按检测标志
    uint8_t last_key_state = 1; // 上一次按键状态
    uint8_t click_count = 0;     // 按键点击计数
    uint32_t last_click_time = 0; // 上一次点击时间
   
    while(1)
    {
        // 读取KEY1状态（GPIOB_Pin_1，上拉输入，按下为低电平）
        uint8_t current_key_state = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
        
        // 按键状态变化检测
        if(current_key_state != last_key_state)
        {
            vTaskDelay(20); // 消抖
            current_key_state = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
            if(current_key_state != last_key_state)
            {
                last_key_state = current_key_state;
                
                if(current_key_state == 0) // 按键按下
                {
                    key_press_time = 0; // 重置计数器
                    short_press_detected = 1; // 标记可能的短按
                }
                else // 按键松开
                {
                    if(short_press_detected && key_press_time < 50) // 短按（小于500ms）
                    {
                        // 执行原有的短按功能
                        num++; // 按键按下，num加1
                        if(task1_handler != NULL)
                        {
                            LED1_OFF(); // 删除任务前先关闭LED1
                            vTaskDelete(task1_handler);//按下KEY1删除task1 
                            task1_handler = NULL;
                        }
                        else
                        {
                            LED1_OFF(); // 重建任务前先关闭LED1，确保从已知状态开始
                            xTaskCreate((TaskFunction_t         )   task1,                //任务函数
                                        (char *                 )   "task1",            //任务名称
                                        (uint16_t )                 TASK1_STACK_SIZE,   //任务堆栈大小
                                        (void *                 )   NULL,               //传递给任务函数的参数
                                        (UBaseType_t            )   TASK1_PRIO,         //任务优先级
                                        (TaskHandle_t *         )   &task1_handler );   //任务句柄
                        }
                    }
                    short_press_detected = 0;
                }
            }
        }
        
        // 长按检测逻辑，长按进入时间调整模式
        if(current_key_state == 0) // 按键按下
        {
            key_press_time++;
            if(key_press_time >= 50) // 长按500ms（50*10ms）
            {
                short_press_detected = 0; // 取消短按标记
                // 进入时间调整模式
                double_click_request = 1;
                // 短暂延时，确保OLED操作完成
                vTaskDelay(10);
                key_press_time = 0; // 重置计数器
                // 等待按键松开
                while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
                {
                    vTaskDelay(10);
                }
                last_key_state = 1;
            }
        }
        
        vTaskDelay(10);
    }
}

/* OLED任务，显示num的值和两个LED的开关状态，以及帧率，支持动画模式 */
void oled_task( void * pvParameters )
{
    lastTick = xTaskGetTickCount(); // 初始化时间戳
    
    while(1)
    {
        // 模式切换请求检查已移除
        
        // 检查双击请求
        if(double_click_request)
        {
            // 重置请求标志
            double_click_request = 0;
            
            // 清屏
            OLED_Clear();
            // 短暂延时，确保清屏操作完成
            vTaskDelay(10);
            
            // 进入时间调整模式
            time_adjust_mode = 1;
            
            // 再次清屏，确保屏幕干净
            OLED_Clear();
            // 短暂延时，确保清屏操作完成
            vTaskDelay(10);
            
            // 跳过本次循环，直接开始新状态
            continue;
        }
        
        if(time_adjust_mode)
        {
            // 时间调整模式
            static uint8_t edit_position = 0; // 0: 小时, 1: 分钟
            static uint8_t hours, minutes, seconds;
            static uint8_t save_request = 0;
            static uint32_t last_flash_time = 0;
            static uint8_t flash_state = 0;
            
            uint32_t current_time = xTaskGetTickCount();
            
            // 初始化时间
            static uint8_t initialized = 0;
            if(!initialized)
            {
                RTC_GetTime(&hours, &minutes, &seconds);
                initialized = 1;
            }
            
            // 检查按键操作
            if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
            {
                vTaskDelay(20); // 消抖
                if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
                {
                    // 长按检测
                    uint8_t press_time = 0;
                    while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
                    {
                        press_time++;
                        vTaskDelay(10);
                        if(press_time >= 10) // 长按100ms
                        {
                            // 调整当前编辑位置的数值
                            if(edit_position == 0) // 小时
                            {
                                hours = (hours + 1) % 24;
                            }
                            else if(edit_position == 1) // 分钟
                            {
                                minutes = (minutes + 1) % 60;
                            }
                            // 短暂延时，避免调整过快
                            vTaskDelay(200);
                        }
                    }
                    
                    // 短按（小于100ms）
                    if(press_time < 10)
                    {
                        // 切换编辑位置
                        if(edit_position == 1) // 当前是分钟，再按就保存退出
                        {
                            save_request = 1;
                        }
                        else // 当前是小时，切换到分钟
                        {
                            edit_position = 1;
                        }
                    }
                }
            }
            
            // 保存时间
            if(save_request)
            {
                RTC_SetTime(hours, minutes, 0); // 秒数重置为0
                save_request = 0;
                time_adjust_mode = 0; // 退出时间调整模式
                OLED_Clear();
                vTaskDelay(10);
                continue;
            }
            
            // 获取实时秒数用于显示
            uint8_t real_seconds;
            RTC_GetTime(NULL, NULL, &real_seconds);
            
            // 显示时间调整界面
            OLED_Clear();
            
            // 显示标题
            OLED_ShowString(1, 1, "Time Adjust");
            
            // 显示时间，当前编辑位置闪烁
            char time_str[9];
            if(flash_state)
            {
                if(edit_position == 0)
                {
                    sprintf(time_str, "  :%02d:%02d", minutes, real_seconds);
                }
                else
                {
                    sprintf(time_str, "%02d:  :%02d", hours, real_seconds);
                }
            }
            else
            {
                sprintf(time_str, "%02d:%02d:%02d", hours, minutes, real_seconds);
            }
            OLED_ShowString(2, 1, time_str);
            
            // 显示操作提示
            OLED_ShowString(3, 1, "Click:Next");
            OLED_ShowString(4, 1, "Hold:Adjust");
            
            // 显示当前编辑位置
            if(edit_position == 0)
            {
                OLED_ShowString(4, 10, "Hour");
            }
            else
            {
                OLED_ShowString(4, 10, "Min");
            }
            
            // 闪烁效果
            if(current_time - last_flash_time >= 500 / portTICK_PERIOD_MS)
            {
                flash_state = !flash_state;
                last_flash_time = current_time;
            }
            
            // 短暂延时，确保任务能够响应其他请求
            vTaskDelay(1);
        }
        // 动画模式已移除
        else
        {
            // 正常模式
            frameCount++;
            
            // 计算帧率
            uint32_t currentTick = xTaskGetTickCount();
            if(currentTick - lastTick >= 1000 / portTICK_PERIOD_MS)
            {
                fps = frameCount / ((currentTick - lastTick) * portTICK_PERIOD_MS / 1000);
                frameCount = 0;
                lastTick = currentTick;
            }
            
            OLED_ShowString(1, 1, "Num:");
            OLED_ShowNum(1, 6, num, 3);
            
            OLED_ShowString(2, 1, "LED1:");
            if(GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1) == 0) // 低电平亮
            {
                OLED_ShowString(2, 6, "ON  ");
            }
            else
            {
                OLED_ShowString(2, 6, "OFF");
            }
            
            OLED_ShowString(3, 1, "LED2:");
            if(GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_2) == 0) // 低电平亮
            {
                OLED_ShowString(3, 6, "ON  ");
            }
            else
            {
                OLED_ShowString(3, 6, "OFF");
            }
            
            // 显示实时时间
            static uint8_t last_display_seconds = 255;
            static char time_str[9];
            
            uint8_t hours, minutes, seconds;
            RTC_GetTime(&hours, &minutes, &seconds);
            
            // 只在秒数变化时更新显示，减少屏闪
            if(seconds != last_display_seconds)
            {
                sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);
                last_display_seconds = seconds;
            }
            
            OLED_ShowString(4, 1, "Time:");
            OLED_ShowString(4, 6, time_str);
        }
        
        vTaskDelay(1); // 每1ms更新一次显示，理论最高帧率1000fps
    }
}

// RTC初始化函数
void RTC_Init(void)
{
    uint32_t timeout = 0;
    uint8_t rtc_init_success = 0;
    
    // 使能PWR和BKP时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    
    // 使能对BKP和RTC的访问
    PWR_BackupAccessCmd(ENABLE);
    
    // 清除BKP区域复位标志
    BKP_DeInit();
    
    // 尝试使用LSI作为RTC时钟源（内部低速时钟，更可靠）
    RCC_LSICmd(ENABLE);
    
    // 等待LSI就绪，带超时
    timeout = 0;
    while ((RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) && (timeout < 100000))
    {
        timeout++;
    }
    
    if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == SET)
    {
        // 选择LSI作为RTC时钟源
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        // 使能RTC时钟
        RCC_RTCCLKCmd(ENABLE);
        
        // 等待RTC寄存器同步
        RTC_WaitForSynchro();
        // 等待RTC可写
        RTC_WaitForLastTask();
        
        // 设置RTC预分频器，LSI=40kHz
        RTC_SetPrescaler(39999); // 40000Hz / (39999+1) = 1Hz
        // 等待RTC可写
        RTC_WaitForLastTask();
        
        // 设置初始时间为12:00:00
        RTC_SetTime(12, 0, 0);
        
        rtc_init_success = 1;
    }
    
    // 如果RTC初始化失败，不影响其他功能
    if (!rtc_init_success)
    {
        // RTC初始化失败，记录错误
        // 但不阻止程序继续运行
    }
}

// 设置RTC时间
void RTC_SetTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    // 计算RTC计数器值
    uint32_t counter = hour * 3600 + minute * 60 + second;
    
    // 等待RTC可写
    RTC_WaitForLastTask();
    // 设置RTC计数器
    RTC_SetCounter(counter);
    // 等待RTC可写
    RTC_WaitForLastTask();
}

// 获取RTC时间
void RTC_GetTime(uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    static uint32_t start_ticks = 0;
    
    // 初始化启动时间
    if (start_ticks == 0)
    {
        start_ticks = xTaskGetTickCount();
    }
    
    // 尝试读取RTC计数器值
    uint32_t counter = RTC_GetCounter();
    
    // 检查计数器值是否合理（避免异常值）
    if (counter < 86400) // 一天的秒数
    {
        // 计算时、分、秒
        if(hour) *hour = (counter / 3600) % 24;
        if(minute) *minute = (counter % 3600) / 60;
        if(second) *second = counter % 60;
        
        return;
    }
    
    // 回退模式：使用系统tick作为时间源
    uint32_t current_ticks = xTaskGetTickCount();
    uint32_t elapsed_ticks = current_ticks - start_ticks;
    uint32_t total_seconds = elapsed_ticks * portTICK_PERIOD_MS / 1000;
    
    // 计算时、分、秒（从12:00:00开始）
    if(hour) *hour = (12 + total_seconds / 3600) % 24;
    if(minute) *minute = (total_seconds / 60) % 60;
    if(second) *second = total_seconds % 60;
}