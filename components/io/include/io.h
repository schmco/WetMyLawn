#ifndef _IO_H_
#define _IO_H_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "soc/gpio_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP32_WROOM

static const uint64_t IO_STRAPPING_MASK = 
    (1ULL << 0) | (1ULL << 2) | (1ULL << 5) | (1ULL << 12) | (1ULL << 15); //Strapping Pins

static const uint64_t IO_UART_MASK = 
    (1ULL << 1) | (1ULL << 3); // UART

#ifdef ESP32_WROOM
static const uint64_t IO_SPIFLASH_MASK = 
    (0x3FULL << 6);  // 6-11 SPI flash
#else
static const uint64_t IO_SPIFLASH_MASK = 
    (0x3FULL << 6) | (0x3ULL << 16);  // 6-11 & 16/17 SPI flash
#endif
static const uint64_t IO_JTAG_MASK = 
    (0xFULL << 12);    // 12-15 JTAG

static const uint64_t IO_INIT_RESERVED_MASK = 
    IO_STRAPPING_MASK | IO_UART_MASK | IO_SPIFLASH_MASK | IO_JTAG_MASK;

static const uint64_t IO_CFG_RESERVED_MASK = 
    IO_UART_MASK | IO_SPIFLASH_MASK | IO_JTAG_MASK;   

static const uint64_t IO_INPUT_ONLY = 
    (0x3FULL << 34);    // 34-39 Input Only


typedef enum{
    IO_SELECT_GPIO,
    IO_SELECT_ADC,
    IO_SELECT_RTC,
    IO_SELECT_INTR,
}io_select_e;

typedef struct {
    gpio_num_t pin;
    gpio_mode_t mode;
    gpio_pulldown_t pull_down;
    gpio_pullup_t pull_up;
    gpio_int_type_t intr_type;
}io_config_s;

esp_err_t io_init(void);
esp_err_t io_cfg(const io_config_s *config);
esp_err_t io_set_level(const io_config_s *config, uint8_t gpio_state);
int8_t io_get_level(const io_config_s *config);

esp_err_t io_configure_pin_interrupt(const io_config_s *config, uint32_t io_evt_queue_lngth);
esp_err_t io_deconfigure_pin_interrupt(const io_config_s *config);
bool io_wait_for_event(gpio_num_t pin, uint32_t *p_pin_buffer, TickType_t wait_ticks);

#ifdef __cplusplus
}
#endif

#endif /* _IO_H_ */