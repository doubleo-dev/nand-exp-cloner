#include <stdio.h>
#include "pcf857x.h"
#include "pcf857x_params.h"
#include "periph/i2c.h"
#include "ztimer.h"

#define I2C_DEVICE I2C_DEV(0)
#define PCF8574_ADDR 0x7    // PCF8574 LCD 어댑터 주소 (0x20 + 7)
#define BACKLIGHT 0x08      // 백라이트 ON 비트
#define ENABLE    0x04      // Enable 비트
#define RS        0x01      // RS 비트

static pcf857x_t pcf;

/**
 * PCF8574로 1바이트 전송
 */
void lcd_write_byte(uint8_t data) {
    pcf857x_port_write(&pcf, data | BACKLIGHT);
}

/**
 * Enable 펄스 (E high -> low)
 */
void lcd_enable(uint8_t data) {
    lcd_write_byte(data | ENABLE);
    ztimer_sleep(ZTIMER_USEC, 1);  // E high hold time
    lcd_write_byte(data & ~ENABLE);
    ztimer_sleep(ZTIMER_USEC, 50); // 실행 대기
}

/**
 * 4비트 전송
 */
void lcd_send_nibble(uint8_t nibble, uint8_t control) {
    uint8_t data = (nibble << 4) | control;
    lcd_enable(data);
}

/**
 * 명령어/데이터 1바이트 전송
 */
void lcd_send_byte(uint8_t byte, uint8_t mode) {
    lcd_send_nibble(byte >> 4, mode);
    lcd_send_nibble(byte & 0x0F, mode);
}

/**
 * LCD 명령 전송
 */
void lcd_cmd(uint8_t cmd) {
    lcd_send_byte(cmd, 0x00); // RS=0
}

/**
 * LCD 데이터 전송
 */
void lcd_data(uint8_t data) {
    lcd_send_byte(data, RS); // RS=1
}

/**
 * LCD 초기화
 */
void lcd_init(void) {
    ztimer_sleep(ZTIMER_USEC, 50000); // Power on 대기

    // 4비트 초기화 시퀀스
    lcd_send_nibble(0x03, 0x00);
    ztimer_sleep(ZTIMER_USEC, 4500);
    lcd_send_nibble(0x03, 0x00);
    ztimer_sleep(ZTIMER_USEC, 4500);
    lcd_send_nibble(0x03, 0x00);
    ztimer_sleep(ZTIMER_USEC, 150);
    lcd_send_nibble(0x02, 0x00); // 4비트 모드
    ztimer_sleep(ZTIMER_USEC, 150);

    // Function set: 2-line, 5x7
    lcd_cmd(0x28);
    // Display ON
    lcd_cmd(0x0C);
    // Clear display
    lcd_cmd(0x01);
    ztimer_sleep(ZTIMER_USEC, 2000);
    // Entry mode
    lcd_cmd(0x06);
}

/**
 * 문자열 출력
 */
void lcd_print(const char* str) {
    while (*str) {
        lcd_data((uint8_t)(*str++));
    }
}

/**
 * 메인
 */
int main(void) {
    puts("PCF8574 LCD 테스트 시작");

    // PCF8574 초기화
    pcf857x_params_t params = {
        .dev  = I2C_DEVICE,
        .addr = PCF8574_ADDR,
        .exp  = PCF857X_EXP_PCF8574
    };

    if (pcf857x_init(&pcf, &params) != 0) {
        puts("PCF8574 초기화 실패!");
        return 1;
    }

    // LCD 초기화
    lcd_init();

    // LCD 출력
    lcd_print("doubleO Co., Ltd.");

    puts("LCD 출력 완료");
    puts("PCF8574 LCD 테스트 종료");

    return 0;
}
