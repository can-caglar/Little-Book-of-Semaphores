#include "cc_led.h"

static void _init_gpio(GPIO_TypeDef* port, uint32_t pin);

void led_init(void)
{
  HAL_Init();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  _init_gpio(LED_PORT, RED_LED_PIN);
  _init_gpio(LED_PORT, ORANGE_LED_PIN);
  _init_gpio(LED_PORT, GREEN_LED_PIN);
  _init_gpio(LED_PORT, BLUE_LED_PIN);
}

void led_on(uint32_t led)
{
  HAL_GPIO_WritePin(LED_PORT, led, GPIO_PIN_SET);
}

void led_off(uint32_t led)
{
  HAL_GPIO_WritePin(LED_PORT, led, GPIO_PIN_RESET);
}

void led_toggle(uint32_t led)
{
  HAL_GPIO_TogglePin(LED_PORT, led);
}

static void _init_gpio(GPIO_TypeDef* port, uint32_t pin)
{
  GPIO_InitTypeDef led;

  led.Pin = pin;
  led.Mode = GPIO_MODE_OUTPUT_PP;
  led.Pull = GPIO_NOPULL;
  led.Alternate = 0;
  led.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(port, &led);
}

