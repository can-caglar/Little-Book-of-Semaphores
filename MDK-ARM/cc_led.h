#ifndef cc_led_included
#define cc_led_included

#include "main.h"
#include "stm32f4xx_hal.h"

#define LED_PORT        GPIOD
#define ORANGE_LED_PIN  GPIO_PIN_13
#define GREEN_LED_PIN   GPIO_PIN_12
#define RED_LED_PIN     GPIO_PIN_14
#define BLUE_LED_PIN    GPIO_PIN_15

void led_init(void);
void led_on(uint32_t led);
void led_off(uint32_t led);
void led_toggle(uint32_t led);

#endif
