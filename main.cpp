/*
 * Copyright (c) 2020 Embedded Planet
 * Copyright (c) 2020 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"

#include "bootutil/bootutil.h"
#include "bootutil/image.h"
#include "FlashIAP/FlashIAPBlockDevice.h"
#include "drivers/InterruptIn.h"

#define TRACE_GROUP "main"
#include "mbed-trace/mbed_trace.h"

mbed::BlockDevice* get_secondary_bd(void) {
    mbed::BlockDevice* default_bd = mbed::BlockDevice::get_default_instance();
    static mbed::SlicingBlockDevice sliced_bd(default_bd, 0x0, MCUBOOT_SLOT_SIZE);
    return &sliced_bd;
}

int main()
{
    // Enable traces from relevant trace groups
    mbed_trace_init();
    mbed_trace_include_filters_set("main,MCUb,BL");

    InterruptIn btn(DEMO_BUTTON);

    // Check if an update has been performed
    int swap_type = boot_swap_type();
    int ret;
    switch(swap_type) {
        case BOOT_SWAP_TYPE_NONE:
            tr_info("Regular boot");
            break;

        case BOOT_SWAP_TYPE_REVERT:
            // After MCUboot has swapped a (non-permanent) update image
            // into the primary slot, it defaults to reverting the image on the NEXT boot.
            // This is why we see "[INFO][MCUb]: Swap type: revert" which can be misleading.
            // Confirming the CURRENT boot dismisses the reverting.
            tr_info("Firmware update applied successfully");

            // Do whatever is needed to verify the firmware is okay (eg: self test)
            // then mark the update as successful. Here we let the user press a button.
            tr_info("Press the button to confirm, or reboot to revert the update");
#if DEMO_BUTTON_ACTIVE_LOW
            while (btn) {
#else
            while (!btn) {
#endif
                sleep();
            }

            ret = boot_set_confirmed();
            if (ret == 0) {
                tr_info("Current firmware set as confirmed");
                return 0;
            } else {
                tr_error("Failed to confirm the firmware: %d", ret);
            }
            break;

        // Note: Below are intermediate states of MCUboot and
        // should never reach the application...
        case BOOT_SWAP_TYPE_FAIL:   // Unable to boot due to invalid image
        case BOOT_SWAP_TYPE_PERM:   // Permanent update requested (when signing the image) and to be performed
        case BOOT_SWAP_TYPE_TEST:   // Revertable update requested and to be performed
        case BOOT_SWAP_TYPE_PANIC:  // Unrecoverable error
        default:
            tr_error("Unexpected swap type: %d", swap_type);
    }

    // Erase secondary slot
    // On the first boot, the secondary BlockDevice needs to be clean
    // If the first boot is not normal, please run the erase step, then reboot

    tr_info("> Press button to erase secondary slot");

#if DEMO_BUTTON_ACTIVE_LOW
    while (btn) {
#else
    while (!btn) {
#endif
        sleep();
    }

    BlockDevice *secondary_bd = get_secondary_bd();
    ret = secondary_bd->init();
    if (ret == 0) {
        tr_info("Secondary BlockDevice inited");
    } else {
        tr_error("Cannot init secondary BlockDevice: %d", ret);
    }

    tr_info("Erasing secondary BlockDevice...");
    ret = secondary_bd->erase(0, secondary_bd->size());
    if (ret == 0) {
        tr_info("Secondary BlockDevice erased");
    } else {
        tr_error("Cannot erase secondary BlockDevice: %d", ret);
    }

    tr_info("> Press button to copy update image to secondary BlockDevice");

#if DEMO_BUTTON_ACTIVE_LOW
    while (btn) {
#else
    while (!btn) {
#endif
        sleep();
    }

    // Copy the update image from internal flash to secondary BlockDevice
    // This is a "hack" that requires you to preload the update image into `mcuboot.primary-slot-address` + 0x40000

    FlashIAPBlockDevice fbd(MCUBOOT_PRIMARY_SLOT_START_ADDR + 0x40000, 0x20000);
    ret = fbd.init();
    if (ret == 0) {
        tr_info("FlashIAPBlockDevice inited");
    } else {
        tr_error("Cannot init FlashIAPBlockDevice: %d", ret);
    }

    static uint8_t buffer[0x1000];
    for (size_t offset = 0; offset < 0x20000; offset+= sizeof(buffer)) {
        ret = fbd.read(buffer, offset, sizeof(buffer));
        if (ret != 0) {
            tr_error("Failed to read FlashIAPBlockDevice at offset %u", offset);
        }
        ret = secondary_bd->program(buffer, offset, sizeof(buffer));
        if (ret != 0) {
            tr_error("Failed to program secondary BlockDevice at offset %u", offset);
        }
    }

    // Activate the image in the secondary BlockDevice

    tr_info("> Image copied to secondary BlockDevice, press button to activate");

#if DEMO_BUTTON_ACTIVE_LOW
    while (btn) {
#else
    while (!btn) {
#endif
        sleep();
    }

    ret = boot_set_pending(false);
    if (ret == 0) {
        tr_info("> Secondary image pending, reboot to update");
    } else {
        tr_error("Failed to set secondary image pending: %d", ret);
    }
}
