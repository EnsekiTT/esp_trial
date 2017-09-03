/*
PWMを発生させてLEDの明るさを変えたり、
オシロスコープで値の変化を確認します。
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/xtensa_api.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "esp_attr.h"
#include "esp_err.h"

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CH0_GPIO       (4)
#define LEDC_CH0_CHANNEL    LEDC_CHANNEL_0

typedef struct {
    int channel;
    int io;
    int mode;
    int timer_idx;
} ledc_info_t;

void app_main()
{
    ledc_timer_config_t ledc_timer = {
        .bit_num = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CH0_CHANNEL,
        .duty = 0,
        .gpio_num = LEDC_CH0_GPIO,
        .intr_type = LEDC_INTR_FADE_END,
        .speed_mode = LEDC_MODE,
        .timer_sel = LEDC_TIMER,
    };
    ledc_channel_config(&ledc_channel);

    while (1) {
        ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 500);
        ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 4000);
        ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
