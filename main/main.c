#include <stdio.h>

//FreeRTOS Essentials
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//ESP system Essentials
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_station.c"

void app_main(void) {

    ESP_LOGI("ESP", "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI("ESP", "IDF version: %s", esp_get_idf_version());

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_OK) ESP_LOGI("ESP", "NVS Initialized");
    else esp_restart();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_sta();
}