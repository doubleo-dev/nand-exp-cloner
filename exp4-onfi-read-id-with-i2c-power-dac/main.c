#define ENABLE_DEBUG 1
#include "debug.h"
#include "periph_cpu.h"
#include "periph_cpu_esp32s3.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "ztimer.h"

#define I2C_DEVICE        I2C_DEV(0)
#define I2C_ADDR_DAC_VCC  (0x4c)      // VCC I2C address
#define I2C_ADDR_DAC_VCCQ (0x4e)      // VCCQ I2C address

#define NAND_NUM_CE (4)
#define NAND_NUM_RB (2)
#define NAND_NUM_DQ (8)

#define NAND_PIN_CE0  (GPIO39)
#define NAND_PIN_CE1  (GPIO40)
#define NAND_PIN_CE2  (GPIO41)
#define NAND_PIN_CE3  (GPIO42)
#define NAND_PIN_RB0  (GPIO48)
#define NAND_PIN_RB1  (GPIO45)
#define NAND_PIN_RE   (GPIO21)
#define NAND_PIN_WE   (GPIO14)
#define NAND_PIN_WP   (GPIO38)
#define NAND_PIN_CLE  (GPIO2)
#define NAND_PIN_ALE  (GPIO1)
#define NAND_PIN_DQ0  (GPIO4)
#define NAND_PIN_DQ1  (GPIO5)
#define NAND_PIN_DQ2  (GPIO6)
#define NAND_PIN_DQ3  (GPIO7)
#define NAND_PIN_DQ4  (GPIO15)
#define NAND_PIN_DQ5  (GPIO16)
#define NAND_PIN_DQ6  (GPIO17)
#define NAND_PIN_DQ7  (GPIO18)

#define BIT31   0x80000000
#define BIT30   0x40000000
#define BIT29   0x20000000
#define BIT28   0x10000000
#define BIT27   0x08000000
#define BIT26   0x04000000
#define BIT25   0x02000000
#define BIT24   0x01000000
#define BIT23   0x00800000
#define BIT22   0x00400000
#define BIT21   0x00200000
#define BIT20   0x00100000
#define BIT19   0x00080000
#define BIT18   0x00040000
#define BIT17   0x00020000
#define BIT16   0x00010000
#define BIT15   0x00008000
#define BIT14   0x00004000
#define BIT13   0x00002000
#define BIT12   0x00001000
#define BIT11   0x00000800
#define BIT10   0x00000400
#define BIT9    0x00000200
#define BIT8    0x00000100
#define BIT7    0x00000080
#define BIT6    0x00000040
#define BIT5    0x00000020
#define BIT4    0x00000010
#define BIT3    0x00000008
#define BIT2    0x00000004
#define BIT1    0x00000002
#define BIT0    0x00000001

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

const gpio_t nand_pins_ce[NAND_NUM_CE] = {
    NAND_PIN_CE0,
    NAND_PIN_CE1,
    NAND_PIN_CE2,
    NAND_PIN_CE3,
};

#define nand_enable_write()      { gpio_write(NAND_PIN_WE, false); }
#define nand_disable_write()     { gpio_write(NAND_PIN_WE, true); }
#define nand_enable_read()       { gpio_write(NAND_PIN_RE, false); }
#define nand_disable_read()      { gpio_write(NAND_PIN_RE, true); }

#define nand_mode_idle()         { \
  gpio_write(NAND_PIN_CLE, false); \
  gpio_write(NAND_PIN_ALE, false); \
}
#define nand_mode_command()      { \
  nand_mode_idle();                \
  gpio_write(NAND_PIN_CLE, true);  \
}
#define nand_mode_address()      { \
  nand_mode_idle();                \
  gpio_write(NAND_PIN_ALE, true);  \
}
#define nand_mode_data()         { \
  nand_mode_idle();                \
}

void nand_voltage_to_dac_array(uint8_t out_arr[3], double voltage) {
  uint16_t dac_value = voltage / 5 * 0xFFFF;  // Convert voltage to DAC value (0-65535 for 16-bit DAC)
  out_arr[0] = 0x10;                          // Control byte : write register & power output
  out_arr[1] = (dac_value >> 8) & 0xFF;       // High byte
  out_arr[2] = dac_value & 0xFF;              // Low byte (not used)
}

void nand_set_dac_voltage(double voltage, uint16_t i2c_dac_addr) {
  uint8_t dac_array[3];
  nand_voltage_to_dac_array(dac_array, voltage);

  i2c_acquire(I2C_DEVICE);
  i2c_write_bytes(I2C_DEVICE, i2c_dac_addr, dac_array, 3, 0);
  i2c_release(I2C_DEVICE);
}

#define nand_set_vcc(voltage)   { \
  nand_set_dac_voltage(voltage, I2C_ADDR_DAC_VCC); \
}

#define nand_set_vccq(voltage)   { \
  nand_set_dac_voltage(voltage, I2C_ADDR_DAC_VCCQ); \
}

void nand_mode_write_dq(void) {
  gpio_init(NAND_PIN_DQ0, GPIO_OUT);
  gpio_init(NAND_PIN_DQ1, GPIO_OUT);
  gpio_init(NAND_PIN_DQ2, GPIO_OUT);
  gpio_init(NAND_PIN_DQ3, GPIO_OUT);
  gpio_init(NAND_PIN_DQ4, GPIO_OUT);
  gpio_init(NAND_PIN_DQ5, GPIO_OUT);
  gpio_init(NAND_PIN_DQ6, GPIO_OUT);
  gpio_init(NAND_PIN_DQ7, GPIO_OUT);
}

void nand_mode_read_dq(void) {
  gpio_init(NAND_PIN_DQ0, GPIO_IN);
  gpio_init(NAND_PIN_DQ1, GPIO_IN);
  gpio_init(NAND_PIN_DQ2, GPIO_IN);
  gpio_init(NAND_PIN_DQ3, GPIO_IN);
  gpio_init(NAND_PIN_DQ4, GPIO_IN);
  gpio_init(NAND_PIN_DQ5, GPIO_IN);
  gpio_init(NAND_PIN_DQ6, GPIO_IN);
  gpio_init(NAND_PIN_DQ7, GPIO_IN);
}

size_t nand_write_to_dq_arr(size_t size, uint8_t dq_arr[]) {
  size_t seq = 0;

  nand_disable_read();
  nand_mode_write_dq();

  while(seq < size) {
    bool dq0 = dq_arr[seq] & BIT0;
    bool dq1 = dq_arr[seq] & BIT1;
    bool dq2 = dq_arr[seq] & BIT2;
    bool dq3 = dq_arr[seq] & BIT3;
    bool dq4 = dq_arr[seq] & BIT4;
    bool dq5 = dq_arr[seq] & BIT5;
    bool dq6 = dq_arr[seq] & BIT6;
    bool dq7 = dq_arr[seq] & BIT7;
    gpio_write(NAND_PIN_DQ0, dq0);
    gpio_write(NAND_PIN_DQ1, dq1);
    gpio_write(NAND_PIN_DQ2, dq2);
    gpio_write(NAND_PIN_DQ3, dq3);
    gpio_write(NAND_PIN_DQ4, dq4);
    gpio_write(NAND_PIN_DQ5, dq5);
    gpio_write(NAND_PIN_DQ6, dq6);
    gpio_write(NAND_PIN_DQ7, dq7);
    nand_enable_write();
    uint8_t dq_byte = (dq7 << 7) | (dq6 << 6) | (dq5 << 5) | (dq4 << 4) | (dq3 << 3) | (dq2 << 2) | (dq1 << 1) | dq0;
    printf("write: %02x: %d%d%d%d %d%d%d%d\n", dq_byte, dq7, dq6, dq5, dq4, dq3, dq2, dq1, dq0);
    nand_disable_write();
    ++seq;
  }

  nand_mode_read_dq();

  return seq;
}

size_t nand_write_to_dq(uint8_t dq_byte) {
  uint8_t dq_arr[1];
  dq_arr[0] = dq_byte;
  return nand_write_to_dq_arr(1, dq_arr);
}

uint8_t nand_read_from_dq(void) {
  //nand_disable_write();
  //nand_mode_read_dq();
  nand_enable_read();
  bool dq0 = gpio_read(NAND_PIN_DQ0);
  bool dq1 = gpio_read(NAND_PIN_DQ1);
  bool dq2 = gpio_read(NAND_PIN_DQ2);
  bool dq3 = gpio_read(NAND_PIN_DQ3);
  bool dq4 = gpio_read(NAND_PIN_DQ4);
  bool dq5 = gpio_read(NAND_PIN_DQ5);
  bool dq6 = gpio_read(NAND_PIN_DQ6);
  bool dq7 = gpio_read(NAND_PIN_DQ7);
  nand_disable_read();

  uint8_t dq_byte = (dq7 << 7) | (dq6 << 6) | (dq5 << 5) | (dq4 << 4) | (dq3 << 3) | (dq2 << 2) | (dq1 << 1) | dq0;
  printf("read: %02x: %d%d%d%d %d%d%d%d\n", dq_byte, dq7, dq6, dq5, dq4, dq3, dq2, dq1, dq0);

  return dq_byte;
}

void nand_disable_channels_all(void) {
  for (uint8_t ch = 0; ch < NAND_NUM_CE; ++ch) {
    gpio_write(nand_pins_ce[ch], true); // Disable all CE_n pins
  }
}

void nand_use_channel(uint8_t channel) {
  for (uint8_t ch = 0; ch < NAND_NUM_CE; ++ch) {
    gpio_write(nand_pins_ce[ch], ch != channel); // Enable CE_n only for selected channel
  }
}

void nand_init(void) {
  nand_set_vcc(0);
  nand_set_vccq(0);

  gpio_init(NAND_PIN_CE0, GPIO_OUT);
  gpio_init(NAND_PIN_CE1, GPIO_OUT);
  gpio_init(NAND_PIN_CE2, GPIO_OUT);
  gpio_init(NAND_PIN_CE3, GPIO_OUT);
  gpio_init(NAND_PIN_RB0, GPIO_IN);
  gpio_init(NAND_PIN_RB1, GPIO_IN);
  gpio_init(NAND_PIN_RE, GPIO_OUT);
  gpio_init(NAND_PIN_WE, GPIO_OUT);
  gpio_init(NAND_PIN_WP, GPIO_IN);
  gpio_init(NAND_PIN_CLE, GPIO_OUT);
  gpio_init(NAND_PIN_ALE, GPIO_OUT);
  gpio_init(NAND_PIN_DQ0, GPIO_OUT);
  gpio_init(NAND_PIN_DQ1, GPIO_OUT);
  gpio_init(NAND_PIN_DQ2, GPIO_OUT);
  gpio_init(NAND_PIN_DQ3, GPIO_OUT);
  gpio_init(NAND_PIN_DQ4, GPIO_OUT);
  gpio_init(NAND_PIN_DQ5, GPIO_OUT);
  gpio_init(NAND_PIN_DQ6, GPIO_OUT);
  gpio_init(NAND_PIN_DQ7, GPIO_OUT);

  nand_disable_channels_all();
  nand_disable_read();
  nand_disable_write();
  nand_mode_idle();

  ztimer_sleep(ZTIMER_MSEC, 5000);  // Wait for 1 second
}

int main(void)
{
  nand_set_vcc(0);
  nand_set_vccq(0);
  for (int sec = 5; sec > 0; --sec) {
    printf("Insert the NAND chip ... %d sec left.\r\n", sec);
    ztimer_sleep(ZTIMER_MSEC, 1000);
  }

  printf("Initialising the NAND flash signals...\r\n");
  nand_init();

  printf("Turning on the NAND flash chip... (3.3V)\r\n");
  nand_set_vcc(3.3);
  nand_set_vccq(3.3);

  printf("Use the channel 0...\r\n");
  nand_use_channel(0);

  printf("READ ID... (cmd: 0x90, addr: 0x00)\r\n");
  nand_mode_command();
  nand_write_to_dq(0x90);        // Read ID command

  nand_mode_address();
  nand_write_to_dq(0x00);        // Read ID address

  nand_mode_data();
  for (int i = 0; i < 30; ++i) {
    //uint8_t byte = nand_read_from_dq();
    //printf("%x ", byte);
    nand_read_from_dq();
  }

  //printf("\r\n");

  while(1) {}

  return 0;
}
