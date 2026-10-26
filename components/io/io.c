#include "io.h"

static QueueHandle_t io_evt_queues[SOC_GPIO_PIN_COUNT] = { NULL };
static gpio_isr_handle_t io_isr_handl = NULL; 
static bool io_isr_registered = pdFALSE;

esp_err_t io_init(void)
{
  esp_err_t err = ESP_OK;
  for (int pin = 0; pin < GPIO_NUM_MAX; pin++)
  {
    if (!GPIO_IS_VALID_GPIO(pin) || ((1ULL << pin) & IO_INIT_RESERVED_MASK))
      continue; // Only reset pins, which are available and can be reseted safely

    err = gpio_reset_pin((gpio_num_t) pin);

    if (err!= ESP_OK) return err;
  }
  
  return err;
}


/* 
TODO: Implement alternate use modes for pins (ADC, INTR, RTC)
*/
esp_err_t io_cfg(const io_config_s *config)
{
  esp_err_t err = ESP_OK;
  if (!GPIO_IS_VALID_GPIO(config->pin)) return ESP_ERR_INVALID_ARG; // Pin is not available
  else if (((1ULL << config->pin) & IO_CFG_RESERVED_MASK)) return ESP_ERR_NOT_SUPPORTED; // Pin is reserved
  else if (((1ULL << config->pin) & IO_INPUT_ONLY) && (config->mode & GPIO_MODE_DEF_OUTPUT)) return ESP_ERR_INVALID_ARG; // Pin is input only, but set to output
   
  err = gpio_reset_pin(config->pin); //reset pin to defined state
  if (err != ESP_OK) return err;

  err = gpio_set_direction(config->pin, config->mode); //set the desired direction of the pin
  if (err != ESP_OK) return err;

  // configure pull up/down resistors
  if (config->pull_up && config->pull_down)  err = gpio_set_pull_mode(config->pin, GPIO_PULLUP_PULLDOWN);
  else if (config->pull_up)                  err = gpio_set_pull_mode(config->pin, GPIO_PULLUP_ONLY);
  else if (config->pull_down)                err = gpio_set_pull_mode(config->pin, GPIO_PULLDOWN_ONLY);
  else                                       err = gpio_set_pull_mode(config->pin, GPIO_FLOATING);
  if (err != ESP_OK) return err;

  if (config->intr_type != GPIO_INTR_DISABLE)
  {
     // configure interrupts
    if (config->intr_type != GPIO_INTR_DISABLE && !(config->mode & GPIO_MODE_DEF_INPUT)) return ESP_ERR_INVALID_ARG;
    if (io_evt_queues[config->pin] == NULL) return ESP_ERR_INVALID_ARG; /* If there is no event que, io_config was used instead of io_configure_pin_interrupt to configure a pin with interrupt. */
    err = gpio_set_intr_type(config->pin,config->intr_type);
    if (err != ESP_OK) return err;
    err = gpio_intr_enable(config->pin);
  }

  return err;
}

esp_err_t io_set_level(const io_config_s *config, uint8_t gpio_state)
{
  if (!(config->mode & GPIO_MODE_DEF_OUTPUT))
    return ESP_ERR_INVALID_ARG;
  return gpio_set_level(config->pin, gpio_state);
}

int8_t io_get_level(const io_config_s *config)
{
  /*
    Note: get level on an output will always return 0
    Since the return value is either 0 or 1 on an gpio pin (NOT an ADC pin)
    we could return max(int) as an error value, if the requested pin is defined as output only
  */

  if (config->mode & (GPIO_MODE_DEF_OUTPUT | GPIO_MODE_DEF_OD | GPIO_MODE_DEF_DISABLE))
    return INT8_MAX;
  return gpio_get_level(config->pin);
}

static void IRAM_ATTR io_global_isr(void* arg)
{

  /*
    uint32_t status         GPIO0~31 interrupt status
    uint32_t status_w1ts;   GPIO0~31 interrupt status write 1 to set
    uint32_t status_w1tc;   GPIO0~31 interrupt status write 1 to clear
  */

  uint32_t st0 = GPIO.status;           /* Statusregister for GPIO 0 .. 31 */
  uint32_t st1 = GPIO.status1.intr_st;  /* Statusregister for GPIO 32 .. 39 */

  GPIO.status_w1tc = st0;              /* clear the interrupt. NOTE: If the interrupt is of type level, it must be cleared after the isr: TODO*/
  GPIO.status1_w1tc.intr_st = st1;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE; /* Used as a flag for imediate contextswitch if during the interrupt a higher prioritized task then the curretn one gets unblocked. */
  while (st0)
  {
    uint32_t pin = __builtin_ctz(st0);
    if (pin < SOC_GPIO_PIN_COUNT && io_evt_queues[pin] != NULL)
    {
      xQueueSendFromISR(io_evt_queues[pin], &pin, &xHigherPriorityTaskWoken); /* From an ISR, pin is send as an item to the end of a que. This happens by value, therefor item should be small, best as a pointer. &xHigherPriorityTaskWoken passes the adress from an ISR Safe function. */
    }
    st0 &= st0 - 1; /* clears the rigthmost set bit in every loop iteration, Brian-Kernighan-Algorithmus. Only #set bits iterrations necessary. */
  }

  while (st1)
  {
    uint32_t pin = 32 + __builtin_ctz(st1);
    if (pin < SOC_GPIO_PIN_COUNT && io_evt_queues[pin] != NULL)
    {
      xQueueSendFromISR(io_evt_queues[pin], &pin, &xHigherPriorityTaskWoken); /* From an ISR, pin is send as an item to the end of a que. This happens by value, therefor item should be small, best as a pointer. &xHigherPriorityTaskWoken passes the adress from an ISR Safe function. */
    }
    st1 &= st1 - 1; /* clears the rigthmost set bit in every loop iteration, Brian-Kernighan-Algorithmus. Only #set bits iterrations necessary. */
  }

  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR(); /* Requesting the yield, if xHigherPriorityTaskWoken did become pdTRUE during our interrupt.  */

}

esp_err_t io_configure_pin_interrupt(const io_config_s *config, uint32_t io_evt_queue_length)
{
  esp_err_t err = ESP_OK;

  if (!GPIO_IS_VALID_GPIO(config->pin)) return ESP_ERR_INVALID_ARG;

  /* Register ISR if not already done */
  if ( io_isr_registered == pdFALSE )
  {
    err = gpio_isr_register(io_global_isr, NULL, ESP_INTR_FLAG_IRAM, &io_isr_handl);
    if (err != ESP_OK) return err;
    io_isr_registered = pdTRUE;
  }

  /* Instantiate queue for this pin interrupt. */
    if (io_evt_queues[config->pin] == NULL) {
        io_evt_queues[config->pin] = xQueueCreate(io_evt_queue_length, sizeof(uint32_t));
        if (io_evt_queues[config->pin] == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

  /* Configure the pin. */
  err = io_cfg(config);
  if (err != ESP_OK)
  {
    vQueueDelete(io_evt_queues[config->pin]);
    io_evt_queues[config->pin] = NULL; /* Delet evt queue if pin configuration is not successfull */
    return err;
  }
  
  return err;
}


esp_err_t io_deconfigure_pin_interrupt(const io_config_s *config)
{
  esp_err_t err = ESP_OK;
  // bool io_evt_queue_reg = pdFALSE;
  if (!GPIO_IS_VALID_GPIO(config->pin)) return ESP_ERR_INVALID_ARG;
  if ( io_evt_queues[config->pin] == NULL ) return ESP_ERR_INVALID_ARG; /* No Interrupt configured for this pin. */
  err = gpio_reset_pin(config->pin);
  if (err!= ESP_OK) return err;
  vQueueDelete(io_evt_queues[config->pin]); 
  /* If a task is waiting in io_wait_for_event with portMAX_DELAY and another task calls io_deconfigure_pin_interrupt, the queue is deleted underneath it, which is undefined behaviour.
  Caller must ensure no task is blocked in io_wait_for_event() on this pin.
  Possible solution: Send sentinel value such as UINT32_MAX to let waiter exit bevor Queue is deleted */
  io_evt_queues[config->pin] = NULL; /* Delet event queue */

  /* TODO: If no event queue is available, every interrupt is deconfigured. We can now unregister our ISR.
  // check if there are any event queues left. If not, deregister ISR. 
  for( int i = 0; i < SOC_GPIO_PIN_COUNT; ++ i ) {
    if( io_evt_queues[i] != NULL ) {
      io_evt_queue_reg = pdTRUE;
      break;
    }
  }
  
  if (!io_evt_queue_reg)
    // TODO: Check how to disable a interrupt with its handl
  */
 return err;
}


bool io_wait_for_event(gpio_num_t pin, uint32_t *p_pin_buffer, TickType_t wait_ticks) {
    if (!GPIO_IS_VALID_GPIO(pin) || io_evt_queues[pin] == NULL) {
        /* check if queue is registered and a valid pin provided */
        return false;
    }
    
    return (xQueueReceive(io_evt_queues[pin], p_pin_buffer, wait_ticks) == pdTRUE);
}