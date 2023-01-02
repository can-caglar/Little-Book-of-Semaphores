/*
Little Book of Semaphores race condition puzzle (1.5.2):

  Puzzle: 
    Suppose that 100 threads increment a global variable X times

    What is the largest possible value of count after all threads have completed?
    What is the smallest possible value?

  Hint: the first question is easy; the second is not.
  
  My Answer: 
    Smallest possible is X, when each thread reads the
    the shared variable before all write to it.
    
    Largest answer is X * 100.
    
    This program attempts to test this by creating 20 threads (not 100
    due to RAM limitations), each incrementing a global variable "gCounter".
    
    Each task sets the task notification flag of another task to indicate
    it is done, after which time it will be suspended. 
    When all threads are done, the global counter value is printed to console.
    
    The compiler switch "NOT_SAFE", when defined will yield the maximum output
    of 2000 (20 * 100) and when not defined, yield the answer 100 as expected.
*/

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "timers.h"
#include "semphr.h"

#define THREAD_COUNT 20
#define TASK_INCREMENT_AMOUNT 100
#define EXPECTED_INCREMENT ((TASK_INCREMENT_AMOUNT) * (THREAD_COUNT))

// When this is defined, program will
// delay in the middle of a critical section
// to exacerbate the race conditions
// When not, it will use macros to protect the critical
// sections.
#define NOT_SAFE

// Global shared variable
uint32_t gCounter;

// Task handles
void vTaskIncrementCounter(void* pvParam);
void vPrintCounterValue(void* pvParam);  // handle for one-shot counter function

// FreeRTOS Objects
SemaphoreHandle_t xMyCountingSem = NULL;
TimerHandle_t xMyTimer = NULL;
TaskHandle_t xPrintTaskHandle = NULL;

uint32_t allTaskFlag = 0;

int main(void)
{
  // Create threads
  for (int i = 0; i < THREAD_COUNT; i++)
  {
    uint32_t thisTaskFlag = (1UL << i);
    BaseType_t err = xTaskCreate(vTaskIncrementCounter, "inc", 0x50, (void*)thisTaskFlag, 2, NULL);
    if (err == pdFAIL)
    {
      printf("Couldn't make task %u\n", i);
    }
    allTaskFlag |= thisTaskFlag;
  }
  
  xTaskCreate(vPrintCounterValue, "print", 0x50, NULL, 1, &xPrintTaskHandle);
  
  // Give control over to scheduler...
  vTaskStartScheduler();
  
  for (;;)
  {
    printf("shouldn't come here\n");
  }
}

void vTaskIncrementCounter(void* pvParam)
{
  for (int i = 0; i < TASK_INCREMENT_AMOUNT; i++)
  {
    uint32_t temp = gCounter;
    
    #ifdef NOT_SAFE
    vTaskDelay(pdMS_TO_TICKS(10));
    gCounter = temp + 1;
    #else
    taskENTER_CRITICAL();
    gCounter = temp + 1;
    taskEXIT_CRITICAL();
    #endif
  }
  xTaskNotify(xPrintTaskHandle, (uint32_t)pvParam, eSetBits);
  vTaskSuspend(NULL);
}

void vPrintCounterValue(void* pvParam)
{  
  uint32_t flags = 0;
 
  for (;;)
  {
    xTaskNotifyWait(pdFALSE, 0x00, &flags, portMAX_DELAY);
    if ((flags & allTaskFlag) == allTaskFlag)
    {
      // all threads are done
      break;
    }
  }
  
  // due to race conditions, the counter value will be less than the expected amount.
  printf("Tasks finished: counter = %d / %d \n", gCounter, EXPECTED_INCREMENT);
  
  vTaskSuspend(NULL);
}
