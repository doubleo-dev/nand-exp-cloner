#include <stdio.h>
#include "periph/i2c.h"
#include "ztimer.h"

#define I2C_DEVICE I2C_DEV(0)
#define I2C_ADDR_DAC_VCC  (0x4c)      // VCC I2C address
#define I2C_ADDR_DAC_VCCQ (0x4e)      // VCCQ I2C address

void voltage_to_dac_array(uint8_t out_arr[3], double voltage) {
  uint16_t dac_value = voltage / 5 * 0xFFFF;  // Convert voltage to DAC value (0-65535 for 16-bit DAC)
  printf("    dac_value :    %5d  /  %5d (%1.10f)\r\n", dac_value, 0xFFFF, (double)dac_value / 0xFFFF * 5);
  out_arr[0] = 0x10;                          // Control byte : write register & power output
  out_arr[1] = (dac_value >> 8) & 0xFF;       // High byte
  out_arr[2] = dac_value & 0xFF;              // Low byte (not used)
}

void set_dac_voltage(double voltage, uint16_t i2c_dac_addr) {
  uint8_t dac_array[3];
  voltage_to_dac_array(dac_array, voltage);
  printf("    dac_array : %02x %02x %02x  >>   i2c (0x%02x)\r\n", dac_array[0], dac_array[1], dac_array[2], i2c_dac_addr);

  i2c_acquire(I2C_DEVICE);
  i2c_write_bytes(I2C_DEVICE, i2c_dac_addr, dac_array, 3, 0);
  i2c_release(I2C_DEVICE);
}

int main(void) {
  for (double voltage = 0; voltage <= 5; voltage += 0.1) {
    printf("Setting VCC and VCCQ to %.1fV ...\r\n", voltage);

    set_dac_voltage(voltage, I2C_ADDR_DAC_VCC);
    set_dac_voltage(voltage, I2C_ADDR_DAC_VCCQ);
    ztimer_sleep(ZTIMER_MSEC, 1000); // Wait for 1 second

    printf("\r\n");
  }

  return 0;
}
