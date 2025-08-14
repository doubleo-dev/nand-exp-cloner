#include <stdio.h>
#include <stdint.h>
#include "periph/i2c.h"
#include "ztimer.h"

#ifndef I2C_DEVICE
#define I2C_DEVICE I2C_DEV(0)
#endif

/* 부작용 방지를 위해 기본 0
 * 1로 켜면 각 주소에 1바이트 쓰기 시도(ACK 여부만 확인) */
#ifndef ALLOW_WRITE_PROBE
#define ALLOW_WRITE_PROBE  0
#endif

static int try_read_probe(i2c_t dev, uint8_t addr)
{
    uint8_t dummy;
    /* 1바이트 읽기 시도: 주소+R에 ACK하면 0 리턴 */
    return i2c_read_bytes(dev, addr, &dummy, 1, 0);
}

static int try_write_probe(i2c_t dev, uint8_t addr)
{
#if ALLOW_WRITE_PROBE
    /* 1바이트 쓰기 시도: 주소+W에 ACK하면 0 리턴
        장치 상태가 바뀔 수 있음. 필요한 경우만 사용 */
    uint8_t dummy = 0x00;
    return i2c_write_bytes(dev, addr, &dummy, 1, 0);
#else
    (void)dev; (void)addr;
    return -1; /* 비활성화 */
#endif
}

void i2c_scan_with_caps(i2c_t dev)
{
    puts("I2C scan: showing capabilities [R?/W?]");

    for (uint8_t addr = 1; addr < 0x7F; addr++) {
        int can_r = 0, can_w = 0;

        i2c_acquire(dev);

        if (try_read_probe(dev, addr) == 0) {
            can_r = 1;
        }
        if (try_write_probe(dev, addr) == 0) {
            can_w = 1;
        }

        i2c_release(dev);

        if (can_r || can_w) {
            const char *cap =
                (can_r && can_w) ? "R/W" :
                (can_r)          ? "R-only" :
                (can_w)          ? "W-only" : "?";
            printf("  0x%02X : %s\n", addr, cap);
        }

        ztimer_sleep(ZTIMER_MSEC, 5);
    }

    puts("I2C scan completed.");
}

int main(void)
{
    i2c_init(I2C_DEVICE);
    i2c_scan_with_caps(I2C_DEVICE);
    return 0;
}
