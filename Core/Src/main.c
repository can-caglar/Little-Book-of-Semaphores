/*
Little Book of Semaphores Exc 3.8, Queue:

  Puzzle: 
      Semaphores can also be used to represent a queue. In this case, the initial value
      is 0, and usually the code is written so that it is not possible to signal unless
      there is a thread waiting, so the value of the semaphore is never positive.
      For example, imagine that threads represent ballroom dancers and that two
      kinds of dancers, leaders and followers, wait in two queues before entering the
      dance floor. When a leader arrives, it checks to see if there is a follower waiting.
      If so, they can both proceed. Otherwise it waits.
      Similarly, when a follower arrives, it checks for a leader and either proceeds
      or waits, accordingly.
      Puzzle: write code for leaders and followers that enforces these constraints.
    
  Code:
      This code models the Leader and Follower scenario mentioned above, where two queues exist,
      one with Followers and other Leaders. Both have to wait until there is someone
      in the other queue before they go up and dance.
      
      The dance "routine" is a function with an internal state that holds the string
      representation of the pair of dancers.

*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Constants
#define LEADER_COUNT  5
#define FOLLOWER_COUNT  5
#define MAX_RANDOM_DELAY_MS  5000
#define MAX_DANCE_DELAY_MS  (MAX_RANDOM_DELAY_MS * 2)
#define DELAY_TOLERANCE ((MAX_RANDOM_DELAY_MS) * 5)

// FreeRTOS objects
SemaphoreHandle_t xDanceFloorMutex;
SemaphoreHandle_t xADancerIsAvailable;
SemaphoreHandle_t xALeaderIsAvailable;

// Strings for printing output
const char* const g_leaders[LEADER_COUNT]     = {"1", "2", "3", "4", "5"};
const char* const g_followers[FOLLOWER_COUNT] = {"a", "b", "c", "d", "e"};

// Tasks
void vLeader(void* pvParam);
void vFollower(void* pvParam);
void vDance(void);

// Helper functions
void vReadyToGoToDanceFloor(char* leader, char* follower);
void vRandomDelay(void);

int main(void)
{
  xDanceFloorMutex = xSemaphoreCreateMutex();
  xADancerIsAvailable = xSemaphoreCreateBinary();
  xALeaderIsAvailable = xSemaphoreCreateBinary();
  
  // create threads representing leaders
  for (UBaseType_t uLeader = 0; uLeader < LEADER_COUNT; uLeader++)
  {
    xTaskCreate(vLeader, (void*)(g_leaders[uLeader]), 0x50, (void*)(g_leaders[uLeader]), 1, NULL);
  }
  
  // create threads representing followers
  for (UBaseType_t uFollower = 0; uFollower < FOLLOWER_COUNT; uFollower++)
  {
    xTaskCreate(vFollower, (void*)(g_followers[uFollower]), 0x50, (void*)(g_followers[uFollower]), 1, NULL);
  }
  
  vTaskStartScheduler();
  
  while(1)
  {
    printf("Shouldn't come here!\n");
    while(1);
  }
}

// LEADERS
void vLeader(void* pvParam)
{
  while(1)
  {
    // do preperation to start dancing
    vRandomDelay(); 
    
    // wait for a follower to give their hand
    printf("%s is waiting to take hand\n", (char*)pvParam);
    if (xSemaphoreTake(xADancerIsAvailable, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
    {
      configASSERT(0);  // should have taken the hand by now!
    }
    printf("%s has taken hand of a follower\n", (char*)pvParam);
    
    // take follower's hand
    xSemaphoreGive(xALeaderIsAvailable);
    
    vReadyToGoToDanceFloor((char*)pvParam, NULL);
    printf("%s is dancing\n", (char*)pvParam);

  }
}

// FOLLOWERS
void vFollower(void* pvParam)
{
  while(1)
  {
    // do preperation to start dancing
    vRandomDelay();  
    
    // bring out hand for a leader
    xSemaphoreGive(xADancerIsAvailable);
    printf("%s is bringing out hand for leader\n", (char*)pvParam);

    // take a leader's hand
    if (xSemaphoreTake(xALeaderIsAvailable, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
    {
      configASSERT(0);  // should have taken the hand by now!
    }
    printf("%s hand has been taken by a leader\n", (char*)pvParam);
    
    // go to dance floor
    vReadyToGoToDanceFloor(NULL, (char*)pvParam);
    printf("%s is dancing\n", (char*)pvParam);

  }
}

// HELPER FUNCTIONS

void vRandomDelay(void)
{
  vTaskDelay(pdMS_TO_TICKS(rand() % MAX_RANDOM_DELAY_MS));
}

void vDance(void)
{
  vTaskDelay(pdMS_TO_TICKS(MAX_DANCE_DELAY_MS));
}

void vReadyToGoToDanceFloor(char* leader, char* follower)
{
  // Function called by follower or dancer thread

  // Static variables keep track of dancers ready to go to dance floor
  static char dancers[2][10];
  static UBaseType_t dancerCount = 0;

  if (xSemaphoreTake(xDanceFloorMutex, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
  {
    configASSERT(0); // should have been table to take mutex by now?
  }
  // Begin critical section
  
  // grab name of leader or follower
  if (leader)
  {
    if (strcmp(dancers[0], "") != 0)
    {
      configASSERT(0); // we already have a leader ready...
    }
    strcpy(dancers[0], leader);
    dancerCount++;
  }
  else if (follower)
  {
    if (strcmp(dancers[1], "") != 0)
    {
      configASSERT(0); // we already have a follower ready...
    }
    strcpy(dancers[1], follower);
    dancerCount++;
  }
  else
  {
    configASSERT(0);  // expected either leader or follower
  }

  if (dancerCount >= 2)
  {
    if (dancerCount > 2)
    {
      configASSERT(0); // didn't expect more than 2 dancers
    }

    printf("%s takes the hand of %s and they both step onto the dance floor!\n", dancers[0], dancers[1]);

    // reset internal state
    dancerCount = 0;
    memset(dancers, 0, 20);
  }

  // End of critical section
  xSemaphoreGive(xDanceFloorMutex);
}
