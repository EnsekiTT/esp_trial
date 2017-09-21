/* MCPWM basic config example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * This example will show you how to use each submodule of MCPWM unit.
 * The example can't be used without modifying the code first.
 * Edit the macros at the top of mcpwm_example_basic_config.c to enable/disable the submodules which are used in the example.
 */

#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#define GPIO_PWM0A_OUT 19   //Set GPIO 19 as PWM0A
#define GPIO_PWM0B_OUT 18   //Set GPIO 18 as PWM0B

typedef struct {
    uint32_t capture_signal;
    mcpwm_capture_signal_t sel_cap_signal;
} capture;

xQueueHandle cap_queue;
#if MCPWM_EN_CAPTURE
static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};
#endif

static void mcpwm_example_gpio_initialize()
{
    printf("initializing mcpwm gpio...\n");
    mcpwm_pin_config_t pin_config = {
        .mcpwm0a_out_num = GPIO_PWM0A_OUT,
        .mcpwm0b_out_num = GPIO_PWM0B_OUT,
    };
    mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);
}

static void gpio_test_signal(void *args)
{
  gpio_config_t gp;
  gp.intr_type = GPIO_INTR_DISABLE;
  gp.mode = GPIO_MODE_OUTPUT;
  gp.pin_bit_mask = GPIO_SEL_13 | GPIO_SEL_12;
  gpio_config(&gp);
  while(1){
    gpio_set_level(GPIO_NUM_13, 1);
    vTaskDelay(10/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_12, 1);
    vTaskDelay(10/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_13, 0);
    vTaskDelay(10/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_12, 0);
    vTaskDelay(10/portTICK_PERIOD_MS);
  }
}

/**
 * @brief Configure whole MCPWM module
 */
static void mcpwm_example_config(void *arg)
{
    //1. mcpwm gpio initialization
    mcpwm_example_gpio_initialize();

    //2. initialize mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;    //frequency = 1000Hz
    pwm_config.cmpr_a = 50.0;       //duty cycle of PWMxA = 50.0%
    pwm_config.cmpr_b = 50.0;       //duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_1;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);   //Configure PWM0A & PWM0B with above settings
    mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_ACTIVE_LOW_MODE, 0, 0);   //Enable deadtime on PWM2A and PWM2B with red = (1000)*100ns on PWM2A
    vTaskDelete(NULL);
}

void app_main()
{
    printf("Testing MCPWM...\n");
    //xTaskCreate(mcpwm_example_config, "mcpwm_example_config", 4096, NULL, 5, NULL);
    xTaskCreate(gpio_test_signal, "gpio_test_signal", 4096, NULL, 5, NULL);
}
