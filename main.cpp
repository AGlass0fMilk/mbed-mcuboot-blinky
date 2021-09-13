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
#include "DataShare.h"	

#define TRACE_GROUP "main"
#include "mbed-trace/mbed_trace.h"

mbed::BlockDevice* get_secondary_bd(void) {
    mbed::BlockDevice* default_bd = mbed::BlockDevice::get_default_instance();
    static mbed::SlicingBlockDevice sliced_bd(default_bd, 0x0, MCUBOOT_SLOT_SIZE);
    return &sliced_bd;
}

static const char *data_share_err_descs[] = {
        "OK",
        "reached EOF",
        "buffer too small",
        "data corrupt"
};

int main()
{
    // Enable traces from relevant trace groups
    mbed_trace_init();
    mbed_trace_include_filters_set("main,MCUb,BL");

#if defined(MCUBOOT_DATA_SHARING)
    /* Print out the shared data */
    DataShare data_share;

    tr_info("data share is %s", data_share.is_valid()? "valid" : "invalid");
    if(data_share.is_valid()) {
        tr_info("data share total size: %u", data_share.get_total_size());
        int result = DATA_SHARE_OK;
        uint16_t type = 0;
        uint8_t buf[128] = {0};
        uint16_t len = 0;
        do {
            result = data_share.get_next(&type, mbed::make_Span(buf), &len);
            if(result == DATA_SHARE_OK) {
                char temp[17];
                tr_info("data share entry: type=0x%X, length=%u", type, len);
                memcpy(temp, buf, 16);
                temp[16] = 0;
                tr_info("\tdata: %s%s", temp, (strlen(temp) == 16)? "..." : "");
            }
        } while(result == DATA_SHARE_OK);

        if(result > 3) {
            result = 3;
        }

        tr_info("done iterating through shared data (reason: %s)", data_share_err_descs[result]);
    }

#endif

    /**
     *  Do whatever is needed to verify the firmware is okay
     *  (eg: self test, connect to server, etc)
     *
     *  And then mark that the update succeeded
     */
    //run_self_test();
    int ret = boot_set_confirmed();
    if (ret == 0) {
        tr_info("Boot confirmed");
    } else {
        tr_error("Failed to confirm boot: %d", ret);
    }

    InterruptIn btn(DEMO_BUTTON);

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
