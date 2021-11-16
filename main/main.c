#include <stdio.h>

//FreeRTOS Essentials
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//ESP system Essentials
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "unifier.h"

void app_main(void) {

    ESP_LOGI("ESP", "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI("ESP", "IDF version: %s", esp_get_idf_version());

    //Initialize NVS 
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_LOGI("ESP", "NVS init failed. Retrying.\n");
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    else ESP_LOGI("ESP", "NVS initialized.\n");
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_sta();
}