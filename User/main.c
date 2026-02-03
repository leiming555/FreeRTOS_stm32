#include "stm32f10x.h"                  // Device header
#include "FreeRTOS_demo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Delay.h"
#include "OLED.h"
#include "KEY.h"
#include "LED.h"


int main(void)
{		
	OLED_Init();
	Key_Init();
	LED_Init();
		
	
	freertos_demo();//RTOS
	
	
}
