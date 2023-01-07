/*
Little Book of Semaphores Exc 3.5, Multiplex:

  Puzzle: 
    Generalize the previous solution so that it allows multiple threads to
    run in the critical section at the same time, but it enforces an upper limit on
    the number of concurrent threads. In other words, no more than n threads can
    run in the critical section at the same time.
    
  Code:
    Below code creates 20 threads and enforces a maximum of 4 threads in a critical section.
    It checks to ensure all 20 threads can access the critical section at least once
    every 10 seconds with the help of a FreeRTOS assert.
*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>

void vThreadA(void* pvParam);

void someCriticalSection(char* taskName);

SemaphoreHandle_t xCriticalSectionKeeper;

static const char* const numbersLookup[] =
{
  "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19"
};

int main(void)
{
  for (int i = 0; i < 20; i++)
  {
    BaseType_t err = xTaskCreate(vThreadA, "A", 0x50, (void*)numbersLookup[i], 1, NULL);
    configASSERT(err == pdPASS);
  }
  
  xCriticalSectionKeeper = xSemaphoreCreateCounting(4, 4);
  
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
    someCriticalSection((char*)pvParam);
  }
}

void someCriticalSection(char* taskName)
{
  static uint8_t threadsInSection = 0;
  TickType_t timeEnteredTask = xTaskGetTickCount();   // timer to decide when to yield task
  
  // Beginning of critical section
  
  BaseType_t err = xSemaphoreTake(xCriticalSectionKeeper, pdMS_TO_TICKS(10000));
  configASSERT(err == pdTRUE);  // a check for starving tasks
  
  // inner critical section, 1 allowed to print!
  portENTER_CRITICAL();
  threadsInSection += 1;
  printf("Task [%s] entered. Threads in section = %u\n", taskName, threadsInSection);
  portEXIT_CRITICAL();
  
  // block a random amount of time- pretending to be some work
  uint32_t msToWait = (rand() % 2000);
  vTaskDelay(pdMS_TO_TICKS(msToWait));
  
  // another inner critical section, 1 allowed to print
  portENTER_CRITICAL();
  threadsInSection -= 1;
  printf("Task [%s] leaving. Threads in section = %u\n", taskName, threadsInSection);
  portEXIT_CRITICAL();
  
  xSemaphoreGive(xCriticalSectionKeeper);
  
  // Exited critical section
  if (xTaskGetTickCount() > timeEnteredTask)  // to prevent yielding too often which wastes processor cycles.
  {
    taskYIELD();  // yield to another task that may be wanting the semaphore
  }
}
