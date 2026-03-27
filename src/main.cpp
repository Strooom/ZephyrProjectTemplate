#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <hexascii.hpp>
#include <qrcode.hpp>

static char hexString[33];  // 8 hex characters + null terminator

int main(void) {

    hexAscii::uint32ToHexString(hexString, 0x12345678);
    while (true) {
        printk("%s\n", hexString);
        k_msleep(1000);
    }

    return 0;
}