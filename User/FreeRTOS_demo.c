#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"
#include "task.h"
#include "LED.h"
#include "KEY.h"
#include "OLED.h"

/*
	KEY.h中的延时消抖需要进行修改
*/

uint8_t KeyNum;
uint8_t num = 0;

// 帧率计算相关变量
uint32_t frameCount = 0;           // 帧数计数器
uint32_t lastTick = 0;             // 上一次计算帧率的系统节拍
uint16_t fps = 0;                  // 帧率值

#define START_TASK_PRIO         1   	//任务优先级
#define START_TASK_STACK_SIZE   128		//任务堆栈大小	
TaskHandle_t    start_task_handler;		//任务句柄
void start_task( void * pvParameters );	//任务函数


#define TASK1_PRIO         2		//任务优先级
#define TASK1_STACK_SIZE   50		//任务堆栈大小	
TaskHandle_t    task1_handler;		//任务句柄
void task1( void * pvParameters );	//任务函数

#define TASK2_PRIO         3		//任务优先级
#define TASK2_STACK_SIZE   50		//任务堆栈大小	
TaskHandle_t    task2_handler;		//任务句柄
void task2( void * pvParameters );	//任务函数

#define TASK3_PRIO         4		//任务优先级
#define TASK3_STACK_SIZE   50		//任务堆栈大小
TaskHandle_t    task3_handler;		//任务句柄
void task3( void * pvParameters );	//任务函数

#define OLED_TASK_PRIO         5		//任务优先级
#define OLED_TASK_STACK_SIZE   200		//任务堆栈大小（增大以容纳动画功能）
TaskHandle_t    oled_task_handler;	//任务句柄
void oled_task( void * pvParameters );	//任务函数

// 动画模式标志
uint8_t animation_mode = 0;
// 模式切换请求标志
uint8_t mode_switch_request = 0;//


void freertos_demo(void)
{    
    xTaskCreate((TaskFunction_t         )   start_task,                //任务函数
                (char *                 )   "start_task",              //任务名称
                (uint16_t )                 START_TASK_STACK_SIZE,	   //任务堆栈大小
                (void *                 )   NULL,			           //传递给任务函数的参数
                (UBaseType_t            )   START_TASK_PRIO,	       //任务优先级
                (TaskHandle_t *         )   &start_task_handler );	   //任务句柄   
				
    vTaskStartScheduler();          //开启任务调度
}


void start_task( void * pvParameters )
{
    taskENTER_CRITICAL();               /* 进入临界区 */
	
    OLED_Init(); // 初始化OLED
    OLED_Clear(); // 清屏
    
    xTaskCreate((TaskFunction_t         )   task1,				//任务函数
                (char *                 )   "task1",            //任务名称
                (uint16_t )                 TASK1_STACK_SIZE,   //任务堆栈大小
                (void *                 )   NULL,               //传递给任务函数的参数
                (UBaseType_t            )   TASK1_PRIO,         //任务优先级
                (TaskHandle_t *         )   &task1_handler );   //任务句柄   
                
    xTaskCreate((TaskFunction_t         )   task2,				//任务函数
                (char *                 )   "task2",            //任务名称
                (uint16_t )                 TASK2_STACK_SIZE,   //任务堆栈大小
                (void *                 )   NULL,               //传递给任务函数的参数
                (UBaseType_t            )   TASK2_PRIO,         //任务优先级
                (TaskHandle_t *         )   &task2_handler );   //任务句柄   
                
    xTaskCreate((TaskFunction_t         )   task3,				//任务函数
                (char *                 )   "task3",            //任务名称
                (uint16_t )                 TASK3_STACK_SIZE,   //任务堆栈大小
                (void *                 )   NULL,               //传递给任务函数的参数
                (UBaseType_t            )   TASK3_PRIO,         //任务优先级
                (TaskHandle_t *         )   &task3_handler );   //任务句柄   
                
    xTaskCreate((TaskFunction_t         )   oled_task,			//任务函数
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

/* 任务三，判断按键KEY1，按下KEY1删除或重新创建task1，并让num加1，长按切换到动画模式 */
void task3( void * pvParameters )
{
    uint8_t key_press_time = 0; // 按键按下时间计数器
    uint8_t key_state = 0;      // 按键状态
    uint8_t short_press_detected = 0; // 短按检测标志
    uint8_t last_key_state = 1; // 上一次按键状态
   
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
        
        // 长按检测逻辑
        if(current_key_state == 0) // 按键按下
        {
            key_press_time++;
            if(key_press_time >= 50) // 长按500ms（50*10ms）
            {
                short_press_detected = 0; // 取消短按标记
                // 设置模式切换请求标志
                mode_switch_request = 1;
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
    
    // 动画相关变量
    uint32_t anim_lastTick = xTaskGetTickCount();
    uint32_t anim_frameCount = 0;
    uint16_t anim_fps = 0;
    uint8_t animation_pos = 0;
    uint8_t animation_direction = 1;
    
    while(1)
    {
        // 检查模式切换请求
        if(mode_switch_request)
        {
            // 重置请求标志
            mode_switch_request = 0;
            
            // 清屏
            OLED_Clear();
            // 短暂延时，确保清屏操作完成
            vTaskDelay(10);
            
            // 切换模式
            animation_mode = !animation_mode;
            
            // 重置相关变量
            if(animation_mode)
            {
                // 进入动画模式
                anim_lastTick = xTaskGetTickCount();
                anim_frameCount = 0;
                anim_fps = 0;
                animation_pos = 0;
            }
            else
            {
                // 进入正常模式
                lastTick = xTaskGetTickCount();
                frameCount = 0;
                fps = 0;
            }
            
            // 再次清屏，确保屏幕干净
            OLED_Clear();
            // 短暂延时，确保清屏操作完成
            vTaskDelay(10);
            
            // 跳过本次循环，直接开始新状态
            continue;
        }
        
        if(animation_mode)
        {
            // 动画模式
            anim_frameCount++;
            
            // 计算帧率
            uint32_t currentTick = xTaskGetTickCount();
            if(currentTick - anim_lastTick >= 1000 / portTICK_PERIOD_MS)
            {
                anim_fps = anim_frameCount / ((currentTick - anim_lastTick) * portTICK_PERIOD_MS / 1000);
                // 移除帧率限制，显示实际值
                anim_frameCount = 0;
                anim_lastTick = currentTick;
            }
            
            // 只在需要时重绘静态内容
            static uint16_t last_fps = 0;
            if(last_fps != anim_fps)
            {
                // 帧率变化时重绘静态内容
                last_fps = anim_fps;
                
                // 清屏
                OLED_Clear();
                
                // 显示标题
                OLED_ShowString(1, 1, "Animation Mode");
                
                // 显示帧率
                OLED_ShowString(2, 1, "FPS:");
                OLED_ShowNum(2, 6, anim_fps, 3);
            }
            else
            {
                // 帧率未变化，只清除动画区域
                // 清除上一位置的点
                OLED_SetCursor(4, animation_pos);
                OLED_WriteData(0x00);
            }
            
            // 减慢动画速度，使其在高帧率下仍然清晰可见
            static uint16_t animation_step = 0;
            animation_step++;
            
            // 每10个帧率更新一次位置
            if(animation_step >= 10)
            {
                animation_step = 0;
                
                // 计算新位置
                uint8_t new_pos = animation_pos + animation_direction;
                if(new_pos >= 110)
                {
                    new_pos = 0;
                }
                
                // 绘制移动的点
                OLED_SetCursor(4, new_pos);
                OLED_WriteData(0xFF);
                
                // 更新动画位置
                animation_pos = new_pos;
            }
            else
            {
                // 保持当前位置
                OLED_SetCursor(4, animation_pos);
                OLED_WriteData(0xFF);
            }
        }
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
            
            OLED_ShowString(4, 1, "FPS:");
            OLED_ShowNum(4, 6, fps, 3);
        }
        
        vTaskDelay(1); // 每1ms更新一次显示，理论最高帧率1000fps
    }
}

