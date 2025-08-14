#define ENABLE_DEBUG 1
#include "debug.h"
#include "periph_cpu.h"
#include "periph_cpu_esp32s3.h"
#include "periph/gpio.h"
#include "ztimer.h"

#define BUTTON_PIN_1  (GPIO19)
#define BUTTON_PIN_2  (GPIO20)

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void nand_init(void) {
}

int main(void)
{
  gpio_init(BUTTON_PIN_1, GPIO_IN);
  gpio_init(BUTTON_PIN_2, GPIO_IN);

  while(1) {
    bool button1 = gpio_read(BUTTON_PIN_1);
    bool button2 = gpio_read(BUTTON_PIN_2);
    printf("button1> %d | %d <button2\r\n", button1, button2);
    ztimer_sleep(ZTIMER_MSEC, 100);
  }

  return 0;
}
