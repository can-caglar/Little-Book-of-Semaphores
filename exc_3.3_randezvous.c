/*
Little Book of Semaphores Exc 3.3:

  Puzzle: 
    Given two threads, A and B, with two
    functions in each, guarantee that
    a1 happens before b2, and b1 happens
    before a2.
    
    Thread A
    --------
    a1
    a2
    
    Thread B
    b1
    b2
    
    
*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

void vThreadA(void* pvParam);
void vThreadB(void* pvParam);

void a1(void);
void a2(void);
void b1(void);
void b2(void);

void safePrint(char* str);

SemaphoreHandle_t xDone_a1;
SemaphoreHandle_t xDone_b1;

int main(void)
{
  xTaskCreate(vThreadA, "A", 0x100, NULL, 1, NULL);
  xTaskCreate(vThreadB, "B", 0x100, NULL, 1, NULL);
  
  xDone_a1 = xSemaphoreCreateBinary();
  xDone_b1 = xSemaphoreCreateBinary();
  
  vTaskStartScheduler();
  
  for (;;)
  {
    printf("Shouldn't come here\n");
    while(1);
  }
}

void vThreadA(void* pvParam)
{
  for (;;)
  {
    a1();
    a2();
  }
}

void vThreadB(void* pvParam)
{
  for (;;)
  {
    b1();
    b2();
  }
}

void a1(void)
{
  safePrint("A:\t1");
  BaseType_t err = xSemaphoreGive(xDone_a1);
  configASSERT(err == pdTRUE);
}

void a2(void)
{
  BaseType_t err = xSemaphoreTake(xDone_b1, pdMS_TO_TICKS(100));
  configASSERT(err == pdTRUE);
  safePrint("A:\t2");
}

void b1(void)
{
  safePrint("B:\t1");
  xSemaphoreGive(xDone_b1);
}

void b2(void)
{
  BaseType_t err = xSemaphoreTake(xDone_a1, pdMS_TO_TICKS(100));
  configASSERT(err == pdTRUE);
  safePrint("B:\t2");
}

void safePrint(char* str)
{
  portENTER_CRITICAL();
  
  printf("%s\n", str);
  
  portEXIT_CRITICAL();
}
