/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

static const char *TAG = "WiFi_Test";
#define DEFAULT_SSID CONFIG_WIFI_SSID
#define DEFAULT_PWD CONFIG_WIFI_PASSWORD
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

#define WEB_SERVER "ensekitt.hatenablog.com"
#define WEB_PORT 80
#define WEB_URL "http://ensekitt.hatenablog.com/"

static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  switch(event->event_id){
    case SYSTEM_EVENT_STA_START:
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
    	ESP_ERROR_CHECK(esp_wifi_connect());
    	break;
    case SYSTEM_EVENT_STA_GOT_IP:
      xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    	ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
    	ESP_LOGI(TAG, "got ip:%s\n",
    	 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    	ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
    	break;
    default:
      break;
  }
  return ESP_OK;
}

static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.
           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket\r\n");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        close(s);
        ESP_LOGI(TAG, "Success!");
        for(int countdown = 10; countdown >= 0; countdown--) {
          ESP_LOGI(TAG, "%d... ", countdown);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

void app_main()
{
  ESP_ERROR_CHECK( nvs_flash_init() );
  ESP_LOGI(TAG, "WAKE UP!");

  //TCP/IPアダプタの初期化
  tcpip_adapter_init();

  //イベントハンドラの準備
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  //WiFiコンフィグ関連の準備
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  wifi_config_t wifi_config = {
    .sta = {
      .ssid = DEFAULT_SSID,
	    .password = DEFAULT_PWD
    },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

  //WiFiドライバの起動
  ESP_LOGI(TAG, "WIFI_START");
  ESP_ERROR_CHECK(esp_wifi_start());
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  //WiFi接続
  ESP_LOGI(TAG, "WIFI_CONNECT");
  ESP_ERROR_CHECK(esp_wifi_connect());

  //Webへアクセスタスクの実行
  xTaskHandle xHandle;
  xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, &xHandle);
  vTaskDelay(30000 / portTICK_PERIOD_MS);

  //アクセスタスクの削除
  vTaskDelete( xHandle );

  //接続解除
  ESP_LOGI(TAG, "DISCONNECT");
  ESP_ERROR_CHECK(esp_wifi_disconnect());
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  //WiFiドライバの停止
  ESP_LOGI(TAG, "STOP");
  ESP_ERROR_CHECK(esp_wifi_stop());
}
