/*
Little Book of Semaphores Exc 3.6, Barrier:

  Puzzle: 
    The synchronization requirement is that no thread executes critical point
    until after all threads have executed rendezvous.
    
    You can assume that there are n threads and that this value is stored in a
    variable, n, that is accessible from all threads.
    
    When the first n - 1 threads arrive they should block until the nth thread
    arrives, at which point all the threads may proceed.

    Thread
    ------
    <<randezvous>>
    <<critical point>>  // all tasks must have finished their randezvous task
    
  Code:
      The solution code below makes use of a "turnstile" pattern, where there
      is a semaphore take and release in quick succession. 
      In essence, once all tasks are ready (tracked by a global counter) 
      one task will be unblocked, which will unblock the next one, and so on. 
      It's called a turnstile as one thread is allowed to pass at a time, 
      and it can be locked to stop all threads from passing.
      
      A binary semaphore is used as it starts with a value of 0.
      This keeps the turnstile locked, to begin with.
      
      A second turnstile starts as open with the use of a counting semaphore
      initialised with a value of 0 with a maximum of 1.
      
      When turnstile 1 is open, turnstile 2 is closed, and vice versa. 
      The turnstiles are opened only once task synchronisation is achieved
      by checking the task counter. This means all tasks will perform
      either the randezvous or the critical point, and never the two
      mixed together.
      
      In a second iteration of this code, a counting semaphore will be used
      for both turnstiles to signal multiple "unlocks" which the book
      suggests will reduce the number of context switches in some situtions.

*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include <stdio.h>
#include <stdlib.h>

// Task prototype
void vThreadA(void* pvParam);

void someCriticalSection(char* taskName);
void someRandezvous(char* taskName);
void somePretendAction(void);

// Variables for sync event
uint8_t threadCounter;

void incrementThreadCounterSafely(void);
void decrementThreadCounterSafely(void);

// FreeRTOS objects
SemaphoreHandle_t xBarrier;
SemaphoreHandle_t xSecondBarrier;
SemaphoreHandle_t xThreadCounterLock;

void openFirstTurnstile(void);
void openSecondTurnstile(void);

void waitAtFirstTurnstile(void);
void waitAtSecondTurnstile(void);

void giveMultipleSemaphore(SemaphoreHandle_t xSemphrHandle, UBaseType_t count);

// Max threads that will be spawned
#define MAX_THREADS 20
#define MAX_TASK_DELAY_MS 2000

// For printing task info
static const char* const numbersLookup[] =
{
  "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19"
};

int main(void)
{
  for (int i = 0; i < MAX_THREADS; i++)
  {
    BaseType_t err = xTaskCreate(vThreadA, "A", 0x50, (void*)i, 1, NULL);
    configASSERT(err == pdPASS);
  }
  
  xBarrier = xSemaphoreCreateCounting(MAX_THREADS, 0);      // xBarrier is closed to begin with
  xSecondBarrier = xSemaphoreCreateCounting(MAX_THREADS, 0); // xSecondBarrier is open to begin with
  xThreadCounterLock =  xSemaphoreCreateMutex();  // to safely access global counter
  
  vTaskStartScheduler();
  
  for (;;)
  {
    printf("Shouldn't come here\n");
    while(1);
  }
}

void vThreadA(void* pvParam)
{
  // Ensures that the Critical section is always performed
  // once all threads have finished the randezvous with
  // the help of the turnstile pattern. Uses two turnstiles.
  char* taskStr = (void*)numbersLookup[(int)pvParam];
  for (;;)
  {
    someRandezvous(taskStr);
    
    incrementThreadCounterSafely();
    
    waitAtFirstTurnstile();
    
    someCriticalSection(taskStr);
    
    decrementThreadCounterSafely();
    
    waitAtSecondTurnstile();
  }
}

void someCriticalSection(char* taskName)
{
  static uint8_t threadsInSection = 0;  
  // Beginning of "critical section"
  
  // inner critical section, 1 allowed to print!
  portENTER_CRITICAL();
  threadsInSection += 1;
  printf("Task [%s] entered. Threads in section = %u\n", taskName, threadsInSection);
  portEXIT_CRITICAL();
  
  somePretendAction();
  
  // another inner critical section, 1 allowed to print
  portENTER_CRITICAL();
  threadsInSection -= 1;
  printf("Task [%s] leaving. Threads in section = %u\n", taskName, threadsInSection);
  portEXIT_CRITICAL();
}

void someRandezvous(char* taskName)
{
  portENTER_CRITICAL();
  printf("Task [%s] performing randezvous.\n", taskName);
  //somePretendAction();
  portEXIT_CRITICAL();
}

void somePretendAction(void)
{
  // block a random amount of time- pretending to be some work
  uint32_t msToWait = (rand() % MAX_TASK_DELAY_MS);
  vTaskDelay(pdMS_TO_TICKS(msToWait));
}

void incrementThreadCounterSafely(void)
{
  if (xSemaphoreTake(xThreadCounterLock, pdMS_TO_TICKS(5000)) == pdFAIL)
  {
    configASSERT(0);
  }
  threadCounter++;
  
  if (threadCounter == MAX_THREADS)
  {
    openFirstTurnstile();
  }
  
  xSemaphoreGive(xThreadCounterLock);
}

void decrementThreadCounterSafely(void)
{
  // take sem
  if (xSemaphoreTake(xThreadCounterLock, pdMS_TO_TICKS(5000)) == pdFAIL)
  {
    configASSERT(0);
  }
  threadCounter--;
  
  if (threadCounter == 0)
  {
    openSecondTurnstile();
  }
  
  // give
  xSemaphoreGive(xThreadCounterLock);
}

void openFirstTurnstile(void)
{
  // all threadsare ready to pass the furst turnstile
  // open the first turnstile - the secibd shall already be closed by now
  if (xSemaphoreTake(xSecondBarrier, 1) != pdFAIL)
  {
    configASSERT(0);  // second barrier should be closed at this point
  }
  giveMultipleSemaphore(xBarrier, MAX_THREADS);  // xSemaphoreGive(xBarrier);
}

void openSecondTurnstile(void)
{
    // all threads have finished the critical section and are ready to pass the second turnstile
    // open the second barrier - the first shall already be closed by now if all threads are through
    // the first
    if (xSemaphoreTake(xBarrier, 1) != pdFAIL)
    {
      configASSERT(0);  //  First turnstile should be closed at this point
    }
    giveMultipleSemaphore(xSecondBarrier, MAX_THREADS);  // xSemaphoreGive(xSecondBarrier);
}

void waitAtFirstTurnstile(void)
{
  if (xSemaphoreTake(xBarrier, pdMS_TO_TICKS(5000)) == pdFAIL)
  {
    configASSERT(0);
  }
  //xSemaphoreGive(xBarrier);
}

void waitAtSecondTurnstile(void)
{
  if (xSemaphoreTake(xSecondBarrier, pdMS_TO_TICKS(5000)) == pdFAIL)
  {
    configASSERT(0);  // expect all tasks to be done by 5 seconds
  }
  //xSemaphoreGive(xSecondBarrier);
}

void giveMultipleSemaphore(SemaphoreHandle_t xSemphrHandle, UBaseType_t count)
{
  for (UBaseType_t i = 0; i < count; i++)
  {
    xSemaphoreGive(xSemphrHandle);
  }
}

