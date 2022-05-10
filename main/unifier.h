#ifndef UNIFIER_H
#define UNIFIER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <driver/gpio.h>

#include <cJSON.h>

#define NVS_NAMESPACE   "main"

/* FreeRTOS short macros */

#define RTOS_MS(ms) pdMS_TO_TICKS(ms)
#define RTOS_SEC(sec) pdMS_TO_TICKS(sec*1000)

#define TASK_DELAY_MS(ms) vTaskDelay(RTOS_MS(ms))
#define TASK_DELAY_SEC(sec) vTaskDelay(RTOS_SEC(sec))

/* Short numtypes */

#define KILOBYTES   (sizeof(char)*1024)

typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef uint64_t u64_t;

typedef int8_t i8_t;
typedef int16_t i16_t;
typedef int32_t i32_t;
typedef int64_t i64_t;

void wifi_init();

void wifi_sta_set_cred(const char* ssid, const char* password);
void wifi_sta_save_cred();
char* wifi_sta_get_ssid();
char* wifi_sta_get_ip4();
i8_t wifi_sta_get_rssi();
bool wifi_is_ap_active();
void wifi_to_sta();
void wifi_to_ap();
void wifi_to_apsta();

#endif