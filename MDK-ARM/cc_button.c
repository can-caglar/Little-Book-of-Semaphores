// Include the appropriate header files
#include "cc_button.h"

#define BUTTON_PORT GPIOA
#define BUTTON_PIN  GPIO_PIN_0

void button_init(void)
{
  // Enable GPIOA clock
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
  // Declare a GPIO_InitTypeDef structure to configure the button pin
  GPIO_InitTypeDef button_pin;

  // Configure the button pin
  button_pin.Pin = GPIO_PIN_0; // Set the pin to be used
  button_pin.Mode = GPIO_MODE_IT_FALLING; // Set the pin to trigger an interrupt on a falling edge
  button_pin.Pull = GPIO_NOPULL;
  button_pin.Speed = GPIO_SPEED_FREQ_LOW; // Set the speed to low frequency

  // Initialize the button pin using the HAL_GPIO_Init() function
  HAL_GPIO_Init(GPIOA, &button_pin);
  
  // Enable and set up interrupt
  HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);  // 1 higher than configMAX_SYSCALL_INTERRUPT_PRIORITY as per FreeRTOS rules
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}
