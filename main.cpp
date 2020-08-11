/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"

#include "bootutil.h"

// Change which LED and how fast we blink it for update binary
#ifndef MCUBOOT_UPDATE
#define LED LED1
#define BLINKING_RATE     1000ms
#else
#define LED LED2
#define BLINKING_RATE     250ms
#endif

int main()
{

    /**
     *  Do whatever is needed to verify the firmware is okay
     *  (eg: self test, connect to server, etc)
     *
     *  And then mark that the update succeeded
     */
    //run_self_test();
    boot_set_confirmed();

    // Initialise the digital pin LED1 as an output
    DigitalOut led(LED);

    while (true) {
        led = !led;
        ThisThread::sleep_for(BLINKING_RATE);
    }
}
