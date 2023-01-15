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
      representation of the pair of dancers and prints it out to console.
      
      Testing the book's solution for the added constraint that each leader can invoke dance
      concurrently with only 1 follower and vice versa.
      
      - The solution makes use of counters for the number of leader/followers that are in queue.
      - The queue is a semaphore, one for leaders and one for followers.
      - It also makes use of a generic semaphore that guards the "dance floor" so only one
      pair can dance at a time.
      - The generic semaphore is held by one dancer only (the one that came second), but
      it is always released by the Leader.
      - The other dancer which came "early" (meaning it had no one in queue to dance with)
      will join its own queue and release the generic semaphore to allow another dancer to
      either signal it to go to the dance floor, or join the queue as well.
      This behaviour is mirrored for Leader and Follower.
      - The semaphore is always "returned" by the Leader at the end of the dance to ensure 
      it's returned once.
      - The dancers randezvous after the dance so that they finish at the same time before
      the Leader returns the semaphore to give opportunity for another pair to dance.
      
      There are a few "moving parts" in this.
      - The queue for Follower
      - The queue for Leader
      - The generic semaphore (which acts like a multi-task mutex) for only 1 dancer to make
      the action.
      - A randezvous at the end to synchronise dancers.
      
      Scenario:
        - A Leader (doesn't matter leader or follower as it's mirrored) comes in.
        - Grabs the generic semaphore and finds no followers in queue (as expected, it was the
        first to grab the generic semaphore!).
        - It increments the Leader queue counter and joins the queue to wait for Followers.
        - Any other leaders that come will end up with following the same pattern.
        - A Follower comes in and checks counter to see that the queue of leaders is not empty.
        - It signals the Leader right away, and makes its way to the dance floor together
        with the Leader.
        - They both "Randezvous" after the dance to ensure they finish together, then
        the Leader releases the generic semaphore, leaving the "dance floor" ready for
        a new pair.
      
      There are a few interesting ar
*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Constants
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define LEADER_COUNT  5
#define FOLLOWER_COUNT  5
#define MAX_RANDOM_DELAY_MS  5000
#define MAX_DANCE_DELAY_MS  (MAX_RANDOM_DELAY_MS * 2)
#define DELAY_TOLERANCE ((MAX_DANCE_DELAY_MS) * MAX(LEADER_COUNT, FOLLOWER_COUNT))

// FreeRTOS objects
SemaphoreHandle_t xDanceFloorMutex;
SemaphoreHandle_t xRandezvousAfterDanceFromLeader;
SemaphoreHandle_t xRandezvousAfterDanceFromFollower;
SemaphoreHandle_t xADancerIsAvailable;
SemaphoreHandle_t xALeaderIsAvailable;
SemaphoreHandle_t xDebugCounterMutex;

// Strings for printing output
const char* const g_leaders[LEADER_COUNT]     = {"1", "2", "3", "4", "5"};
const char* const g_followers[FOLLOWER_COUNT] = {"a", "b", "c", "d", "e"};

// Global counters
UBaseType_t g_leaderCount = 0;
UBaseType_t g_followerCount = 0;

// Tasks
void vLeader(void* pvParam);
void vFollower(void* pvParam);

// Helper functions
void vRandomDelay(void);
void vDance(char* dancer);
void safelyModify(UBaseType_t* value, int8_t amount);

int main(void)
{
  xDanceFloorMutex = xSemaphoreCreateCounting(1, 1);
  xADancerIsAvailable = xSemaphoreCreateBinary();
  xALeaderIsAvailable = xSemaphoreCreateBinary();
  xRandezvousAfterDanceFromLeader = xSemaphoreCreateBinary();
  xRandezvousAfterDanceFromFollower = xSemaphoreCreateBinary();
  xDebugCounterMutex = xSemaphoreCreateMutex();
  
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
    
    if (xSemaphoreTake(xDanceFloorMutex, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
    {
      configASSERT(0);  // should have taken by now!
    }
    
    if (g_followerCount > 0)
    {
      g_followerCount--;
      // bring out hand for a follower
      xSemaphoreGive(xALeaderIsAvailable);
    }
    else
    {
      g_leaderCount++;
      printf("%s entering queue\n", (char*)pvParam);
      xSemaphoreGive(xDanceFloorMutex);  // End of critical section
      // wait to take a follower's hand
      if (xSemaphoreTake(xADancerIsAvailable, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
      {
        configASSERT(0);  // should have taken the hand by now!
      }
    }
        
    vDance((char*)pvParam);
    // randezvous
    xSemaphoreGive(xRandezvousAfterDanceFromLeader);
    if (xSemaphoreTake(xRandezvousAfterDanceFromFollower, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
    {
      configASSERT(0);  // follower should have been done by now!
    }
    xSemaphoreGive(xDanceFloorMutex);  // allow another pair to begin
  }
}

// FOLLOWERS
void vFollower(void* pvParam)
{
  while(1)
  {
    // do preperation to start dancing
    vRandomDelay();  
    
    if (xSemaphoreTake(xDanceFloorMutex, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
    {
      configASSERT(0);  // should have taken by now!
    }
    
    if (g_leaderCount > 0)
    {
      g_leaderCount--;
      // bring out hand for a leader
      xSemaphoreGive(xADancerIsAvailable);
    }
    else
    {
      g_followerCount++;
      printf("%s entering queue\n", (char*)pvParam);
      xSemaphoreGive(xDanceFloorMutex);  // End of critical section
      // wait to take a leader's hand
      if (xSemaphoreTake(xALeaderIsAvailable, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
      {
        configASSERT(0);  // should have taken the hand by now!
      }
    }
    
    // go to dance floor
    vDance((char*)pvParam);
    // randezvous
    xSemaphoreGive(xRandezvousAfterDanceFromFollower);
    if (xSemaphoreTake(xRandezvousAfterDanceFromLeader, pdMS_TO_TICKS(DELAY_TOLERANCE)) == pdFAIL)
    {
      configASSERT(0);  // follower should have been done by now!
    }
  }
}

// HELPER FUNCTIONS

void vRandomDelay(void)
{
  vTaskDelay(pdMS_TO_TICKS(rand() % MAX_RANDOM_DELAY_MS));
}

void vDance(char* dancer)
{
  static UBaseType_t dancers = 0;
  
  if (xSemaphoreTake(xDebugCounterMutex, pdMS_TO_TICKS(10000)) == pdFAIL)
  {
    configASSERT(0); // couldn't grab mutex in time!
  }
  dancers++;
  printf("%s steps onto the dance floor (people in total: %lu)!\n", dancer, dancers);
  xSemaphoreGive(xDebugCounterMutex);
  
  vRandomDelay(); // dance for a random amount of time
  
  if (xSemaphoreTake(xDebugCounterMutex, pdMS_TO_TICKS(10000)) == pdFAIL)
  {
    configASSERT(0); // couldn't grab mutex in time!
  }
  dancers--;
  printf("%s steps off the dance floor (people in total: %lu)!\n", dancer, dancers);
  xSemaphoreGive(xDebugCounterMutex);
}
