/*
かしこいかわいいえるーちか。
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_system.h"
#include "driver/gpio.h"

void app_main()
{
  printf("LED chika chika!\n");
  gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
  int level = 0;
  while (true) {
    gpio_set_level(GPIO_NUM_4, level);
    printf("now %d\n", level);
    level = !level;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
