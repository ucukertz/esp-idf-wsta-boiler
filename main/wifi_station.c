#include <string.h>

//FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//ESP system
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

//lwip
#include <lwip/err.h>
#include <lwip/sys.h>

#include "unifier.h"

#define ESP_WIFI_SSID "yourssid"
#define ESP_WIFI_PASS "yourpass"

#define WIFI_STA_TAG "Wifi Station"
#define ESP_MAX_WIFI_RETRY 5

static int wifi_retry_num = 0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_retry_num < ESP_MAX_WIFI_RETRY) {
            esp_wifi_connect();
            //wifi_retry_num++; //uncomment to limit amount of retries
            ESP_LOGI(WIFI_STA_TAG, "Retrying to connect to the AP");
        } else {

        }
        ESP_LOGW(WIFI_STA_TAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(WIFI_STA_TAG, "Connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_STA_TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_num = 0;
    }
}

void wifi_init_sta()
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_LOGI(WIFI_STA_TAG, "Connecting to SSID = %s Password = %s", ESP_WIFI_SSID, ESP_WIFI_PASS);

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, ESP_WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, ESP_WIFI_PASS);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_STA_TAG, "wifi_init_sta finished.");
}
