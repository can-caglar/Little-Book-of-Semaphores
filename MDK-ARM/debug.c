#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

void vAssertFailed(char* file, uint32_t line)
{
  taskDISABLE_INTERRUPTS(); 
  printf("Assertion failed, file: %s, line %u\n", file, line);
  for( ;; );
};
