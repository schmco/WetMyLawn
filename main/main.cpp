// #include <cstdio>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "io.h"

#define BTN_DBNC_MS 30

// Setting
/*
io_config_s PIN_RLY1 = {GPIO_NUM_32, GPIO_MODE_OUTPUT, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_DISABLE, GPIO_INTR_DISABLE};
io_config_s PIN_RLY2 = {GPIO_NUM_33, GPIO_MODE_OUTPUT, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_DISABLE, GPIO_INTR_DISABLE};
io_config_s PIN_RLY3 = {GPIO_NUM_25, GPIO_MODE_OUTPUT, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_DISABLE, GPIO_INTR_DISABLE};
io_config_s PIN_RLY4 = {GPIO_NUM_26, GPIO_MODE_OUTPUT, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_DISABLE, GPIO_INTR_DISABLE};

*/

io_config_s PIN_LED1 = {
    .pin = GPIO_NUM_23,
    .mode = GPIO_MODE_OUTPUT,
    .pull_down = GPIO_PULLDOWN_DISABLE,
    .pull_up = GPIO_PULLUP_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};

io_config_s PIN_INTR = {
    .pin = GPIO_NUM_18,
    .mode = GPIO_MODE_INPUT,
    .pull_down = GPIO_PULLDOWN_DISABLE,
    .pull_up = GPIO_PULLUP_ENABLE,
    .intr_type = GPIO_INTR_POSEDGE,
};

io_config_s PIN_RLY1 = {GPIO_NUM_32, GPIO_MODE_OUTPUT, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_DISABLE, GPIO_INTR_DISABLE};

static const uint32_t sleep_time_ms = 1000;

esp_err_t mcu_init(void)
{
  esp_err_t err = ESP_OK; 
  // default state for all pins except do not use pins
  err = io_init();
  if (err != 0)
    printf("Init configuration unsucesfull with err code %s\n", esp_err_to_name(err));
  else
    printf("Configuration sucessfull! Code %s\n", esp_err_to_name(err));

  // Init GPIO Pins
  err = io_configure_pin_interrupt(&PIN_INTR, 10);
  if (err != 0)
    printf("Pin configuration unsucesfull with err code %s\n", esp_err_to_name(err));
  
  err = io_cfg(&PIN_LED1);
  if (err != 0)
    printf("Pin configuration unsucesfull with err code %s\n", esp_err_to_name(err));
  
  err = io_cfg(&PIN_RLY1);
  if (err != 0)
    printf("Pin configuration unsucesfull with err code %s\n", esp_err_to_name(err));

  return err;
}

static void gpio_task_example(void* arg)
{
    uint8_t rly_state = 0;
    uint32_t pin;
    for (;;) {
        if (io_wait_for_event(PIN_INTR.pin, &pin , portMAX_DELAY)) {
          vTaskDelay(pdMS_TO_TICKS(BTN_DBNC_MS));
          while (io_wait_for_event(PIN_INTR.pin, &pin, 0)); /* Drain the queue for events occured during the debounce period. Could be done with xQueueReset, but the handle ist not yet exposed*/
          if (io_get_level(&PIN_INTR) == 1) /* Afte the debounce, the level should be the desired one for the action */
            io_set_level(&PIN_RLY1,rly_state=!rly_state);
        }
        else{
          vTaskDelay( pdMS_TO_TICKS(10) );
        }
    }
}


// ESP_IDF defines main elsewhere. The application is expected to be namend
// app_main
extern "C" void app_main(void) {
  int err = 0;
  uint8_t led_state = 0;

  err = mcu_init();

  xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
  // superloop
  while (1) {
    err = io_set_level(&PIN_LED1, led_state = !led_state);
    if (err != 0)
      printf("Set level on pin %d with err code %s\n", PIN_LED1.pin, esp_err_to_name(err));

    // Print LED state
    printf("LED state %d\n", led_state);

    /*Delay, vTaskDelay expects a number of ticks.
    A tick is a internal counter for the sheduler, so that it knows
    when to run which task.*/
    vTaskDelay(pdMS_TO_TICKS( sleep_time_ms ));
  }
}