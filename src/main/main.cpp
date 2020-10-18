#pragma GCC optimize ("O3")

#include <stdio.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_system.h"
#include "esp_int_wdt.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#include "lv-driver/display.h"
#include "lv-game/lvk.h"
#include "lv-game/scene_main.h"
#include "lv-engine/engine.h"

static TaskHandle_t gameloopTask;

#if lvk_measuring == true
    int64_t startTime;
    int64_t stopTime;
#endif

void gameLoop(void * pvParameters){

    for(;;) {

        // director will call onFrame on current scene
        lvDirector.update();

        // director will draw framebuffer using polyfill
        vTaskSuspendAll(); // begining of critical section
        ili9341_begin_drawing();
        lvDirector.draw();
        ili9341_end_drawing();
        xTaskResumeAll(); // end of critical section

        #if lvk_measuring == true
            startTime = stopTime;
            stopTime = esp_timer_get_time()/1000;
            float diff = (float)(stopTime - startTime);
            measuredFPS = (unsigned short) abs( 1.0 / ((diff)/ 1000.0));
            printf("FPS:%d MEM:%d\n", (int) measuredFPS, esp_get_free_heap_size());
        #endif

        vTaskSuspend(NULL); 

    }

}

static void gameloop_timer_callback(void* arg);

extern "C" void app_main(void){
    
    printf("lvndr app is now running...\n");

    ili9341_init();
    ili9341_clear(0x00);

    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);

    scene_main_setup();
    lvDirector.runScene(SCENE_MAIN);

    const esp_timer_create_args_t gameloop_timer_args = {
        .callback = &gameloop_timer_callback,
        .name = "gameloop"
    };

    esp_timer_handle_t gameloop_timer;
    ESP_ERROR_CHECK(esp_timer_create(&gameloop_timer_args, &gameloop_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(gameloop_timer, 33000));

    xTaskCreatePinnedToCore(
        gameLoop, "gameLoop",
        lvk_display_w * lvk_display_h/2, NULL, 25,
        &gameloopTask, 1
    );
}

static void gameloop_timer_callback(void* arg){
    xTaskResumeFromISR(gameloopTask);
}