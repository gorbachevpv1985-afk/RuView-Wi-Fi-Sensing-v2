#ifndef LED_STRIP_H
#define LED_STRIP_H

#include <stdbool.h>

// Заглушка дефайнов моделей и форматов
#define LED_MODEL_WS2812 0
#define LED_STRIP_COLOR_COMPONENT_FMT_GRB 0

// Заглушка типа хэндла
typedef void* led_strip_handle_t;

// Заглушка структуры конфигурации со всеми полями из main.c
typedef struct {
    int strip_gpio_num;
    int max_leds;
    int led_model;
    int color_component_format;
    struct {
        bool invert_out;
    } flags;
} led_strip_config_t;

// Заглушка RMT структуры со всеми полями из main.c
typedef struct {
    int resolution_hz;
    struct {
        bool with_dma;
    } flags;
} led_strip_rmt_config_t;

// Заглушки функций (возвращают 0, имитируя успешную работу)
inline int led_strip_new_rmt_device(const led_strip_config_t* c, const led_strip_rmt_config_t* r, led_strip_handle_t* h) { return 0; }
inline int led_strip_clear(led_strip_handle_t h) { return 0; }
inline int led_strip_set_pixel(led_strip_handle_t h, int i, int r, int g, int b) { return 0; }
inline int led_strip_refresh(led_strip_handle_t h) { return 0; }

// Заглушки для функций дисплея с правильными типами данных
inline int display_init(void) { return 0; }
inline int display_task_start(void) { return 0; }
inline void display_ui_update(int val) {}
inline bool display_is_active(void) { return false; }

#endif
