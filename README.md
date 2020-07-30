# Mbed OS/mcuboot Blinky example

This example project shows how to configure an Mbed-OS application to be booted by an mcuboot bootloader.

The mcuboot bootloader is a small mbed-os application loaded at the beginning of flash and executed first by the processor. The bootloader looks at available internal/external flash and decides when to carry out updates, handles flashing an update and error recovery, and ultimately jumps to the beginning of the main application.

## Memory Regions Overview

### Bootloader
The bootloader lives in the first region of flash where the processor begins execution. The basic mcuboot bootloader does not implement any interfaces to receive updates. It simply looks at available application "slots". The application (or another bootloader) is responsible for loading application updates into a slot visible to the mcuboot bootloader. Update candidates are typically placed in the "secondary" flash region. See the bootloader readme for more information.

Briefly, the bootloader has a maximum size set by `target.restrict_size` when building the bootloader. In this example the bootloader is restricted to a size of `0x1FC00` bytes. This is way larger than required but allows the bootloader to be built with a debug profile during development. In production the bootloader size should be optimized based on your use case.

### Application Header Info

When deciding what to boot/update, the mcuboot bootloader looks at an installed application's header info (type-length-value formatted data that is prepended to the application binary before the application's start address). It uses this header info to validate there is a bootable image installed in the "slot" and optionally to verify the image's digital signature before booting.

By default, this header is configured to be 1kB in size. This is probably way more than needed in most cases and can be adjusted using the configuration parameter `mcuboot.header_size`. This value should be aligned on 4-byte boundaries.

This header prepended to the application hex during the signing process (explained later).

Please note: the bootloader size should be restricted to ensure it does not collide with the application header info. For example, if the application start address is set to `0x20000` and the header size is `0x400`, the bootloader size should be restricted to at most `0x20000 - 0x400 = 0x1FC00`.

### Primary Application

The primary application is the currently installed, bootable application. In this case it is the typical mbed-os-example-blinky application (plus some mcuboot-specific things). The application start address is configured using `target.mbed_app_start` and the size can be restricted using `target.mbed_app_size`.

### Scratch Space

If mcuboot is configured to perform swap-type updates (ie: the update candidate is written to the main application space while the main application is written to the update candidate space), the end of the flash memory map has several sectors reserved for "scratch space". This is an area of flash where mcuboot can temporarily store pieces of each application as well as update status information in case an unexpected reset/power failure happens during an update. See mcuboot documentation for information on considerations when configuring the size of the scratch space.

Please note: The primary application size should be restricted to ensure it does not collide with the scratch space region. For example, if the application start address is set to `0x20000`, the device flash ends at `0x100000`, and the scratch space size is configured to `0x4000`, the application size should be restricted to `0x100000 - (0x20000 + 0x4000) = 0xDC000`.

The size of the scratch space can be configured using `mcuboot.scratch-size` -- this value **must** be erase-sector aligned (ie: a multiple of the flash's eraseable size).

## Configuration 

There are many configuration options available in mcuboot and these are covered in mcuboot's documentation. This section will go over basic configuration that needs to be done to boot an mbed-os application with mcuboot.

The mcuboot repository is included to allow the application to call some application-related mcuboot functions. One use case is having the application flag itself as "okay" after an update has occurred. This prevents mcuboot from reverting the update on the next boot.

By default, the mcuboot repository/library is configured to build a bootloader, **not** an application. So when building an application with mcuboot, it is important to add the following to your `mbed_app.json` configuration file:

```
"mcuboot.bootloader-build": 0
```

Additionally, the application start location and maximum allowed size should be configured as discussed in the above section, "Memory Regions Overview". A bare-minimum mcuboot application configuration file should look like the below `mbed_app.json` excerpt:

```
{
    "target_overrides": {
        "*": {
            "mcuboot.bootloader-build": 0,
            "target.mbed_app_start": "0x20000",
            "target.mbed_app_size": "0xDC000"
        }
    }
}
```



The example project contains an application that repeatedly blinks an LED on supported [Mbed boards](https://os.mbed.com/platforms/).

You can build the project with all supported [Mbed OS build tools](https://os.mbed.com/docs/mbed-os/latest/tools/index.html). However, this example project specifically refers to the command-line interface tool [Arm Mbed CLI](https://github.com/ARMmbed/mbed-cli#installing-mbed-cli).
(Note: To see a rendered example you can import into the Arm Online Compiler, please see our [import quick start](https://os.mbed.com/docs/mbed-os/latest/quick-start/online-with-the-online-compiler.html#importing-the-code).)

1. [Install Mbed CLI](https://os.mbed.com/docs/mbed-os/latest/quick-start/offline-with-mbed-cli.html).
1. From the command-line, import the example: `mbed import mbed-os-example-blinky`
1. Change the current directory to where the project was imported.

## Application functionality

The `main()` function is the single thread in the application. It toggles the state of a digital output connected to an LED on the board.

## Building and running

1. Connect a USB cable between the USB port on the board and the host computer.
1. Run the following command to build the example project and program the microcontroller flash memory:

    ```bash
    $ mbed compile -m <TARGET> -t <TOOLCHAIN> --flash
    ```

Your PC may take a few minutes to compile your code.

The binary is located at `./BUILD/<TARGET>/<TOOLCHAIN>/mbed-os-example-blinky.bin`.

Alternatively, you can manually copy the binary to the board, which you mount on the host computer over USB.

Depending on the target, you can build the example project with the `GCC_ARM`, `ARM` or `IAR` toolchain. After installing Arm Mbed CLI, run the command below to determine which toolchain supports your target:

```bash
$ mbed compile -S
```

## Expected output
The LED on your target turns on and off every 500 milliseconds after being booted by mcuboot.


## Troubleshooting
If you have problems, you can review the [documentation](https://os.mbed.com/docs/latest/tutorials/debugging.html) for suggestions on what could be wrong and how to fix it.

## Related Links

* [Mbed OS Stats API](https://os.mbed.com/docs/latest/apis/mbed-statistics.html).
* [Mbed OS Configuration](https://os.mbed.com/docs/latest/reference/configuration.html).
* [Mbed OS Serial Communication](https://os.mbed.com/docs/latest/tutorials/serial-communication.html).
* [Mbed OS bare metal](https://os.mbed.com/docs/mbed-os/latest/reference/mbed-os-bare-metal.html).
* [Mbed boards](https://os.mbed.com/platforms/).

### License and contributions

The software is provided under Apache-2.0 license. Contributions to this project are accepted under the same license. Please see contributing.md for more info.

This project contains code from other projects. The original license text is included in those source files. They must comply with our license guide.
