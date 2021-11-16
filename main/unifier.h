#ifndef UNIFIER_H
#define UNIFIER_H

#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define FIRMWARE_VER    "boilerplate"

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

void wifi_init_sta();

#endif