/*
 * Fadecandy DFU Bootloader
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdbool.h>
#include "usb_dev.h"
#include "serial.h"
#include "mk20dx128.h"

extern uint32_t boot_token;
static __attribute__ ((section(".appvectors"))) uint32_t appVectors[64];


static void led_on()
{
    // Set the status LED on PC5, as an indication that we're in bootloading mode.
    PORTC_PCR5 = PORT_PCR_MUX(1) | PORT_PCR_DSE | PORT_PCR_SRE;
    GPIOC_PDDR = 1 << 5;
    GPIOC_PDOR = 1 << 5;
}

static bool test_boot_token()
{
    /*
     * If we find a valid boot token in RAM, the application is asking us explicitly
     * to enter DFU mode. This is used to implement the DFU_DETACH command when the app
     * is running.
     */

    return boot_token == 0x74624346;
}

static bool test_app_missing()
{
    /*
     * If there doesn't seem to be a valid application installed, we always go to
     * bootloader mode.
     */

    uint32_t entry = appVectors[1];
    return entry < 0x00001000 || entry >= 128 * 1024;
}

static bool test_banner_echo()
{
    /*
     * At startup we print this banner out to the serial port.
     * If we see it echo back to us, we enter bootloader mode no matter what.
     * This is intended to be a foolproof way to enter recovery mode, even if other
     * circuitry has been connected to the serial port.
     */

    static char banner[] = "FC-Boot";
    const unsigned bannerLength = sizeof banner - 1;
    unsigned matched = 0;

    // Write banner
    serial_begin(BAUD2DIV(9600));
    serial_write(banner, sizeof banner - 1);

    // Newline is not technically part of the banner, so we can do the RX check
    // at a time when we're sure the other characters have arrived in the RX fifo.
    serial_putchar('\n');
    serial_flush();

    while (matched < bannerLength) {
        if (serial_available() && serial_getchar() == banner[matched]) {
            matched++;
        } else {
            break;
        }
    }

    serial_end();
    return matched == bannerLength;
}

static void app_launch()
{
    // Clear the boot token, so we don't repeatedly enter DFU mode.
    boot_token = 0;

    // XXX Enter application code.
    serial_print("\r\nApp launch time!\r\n");

    while (1) {
        watchdog_refresh();
    }
}

int main()
{
    if (test_banner_echo() || test_app_missing() || test_boot_token()) {

        // We're doing DFU mode!
        led_on();
        dfu_init();
        usb_init();

        // XXX debug
        serial_begin(BAUD2DIV(115200));
        serial_print("\r\nHELLOES!\r\n");
        serial_phex32(WDOG_RSTCNT);

        while (1) {
            watchdog_refresh();
        }
    }

    app_launch();
    return 0;
}
