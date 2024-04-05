# ODROID Smart Power 3

This implementation of the `energymon` interface reads from an [ODROID Smart Power 3](https://wiki.odroid.com/accessory/power_supply_battery/smartpower3) USB device.

> NOTE: If you're using an older generation [ODROID Smart Power](https://wiki.odroid.com/old_product/accessory/odroidsmartpower) device, use the `osp` or `osp-polling` implementation instead.


## Prerequisites

You need an ODROID Smart Power 3 device with a USB connection.

This implementation depends on [osp3](https://github.com/energymon/osp3/).


### Linux Privileges

To use an ODROID Smart Power without needing sudo/root at runtime, set appropriate [udev](https://en.wikipedia.org/wiki/Udev) privileges.

You can give access to a specific group, e.g. `plugdev`, by creating/modifying a `udev` config file like `/etc/udev/rules.d/99-osp3.rules`.
For example, add the following rules:

```
# OROID Smart Power 3
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", GROUP="plugdev"
```

For the new permissions to take effect, the device must be remounted by the kernel - either disconnect and reconnect the device or reboot the system.


## Usage

By default, the `osp3` implementation looks for an ODROID Smart Power 3 device at `/dev/ttyUSB0` on Linux and `/dev/tty.usbserial-210` on macOS.
To override, set the environment variable `ENERGYMON_OSP3_DEV_FILE` to the correct device file.

OSP3 devices support various baud rates: 9600, 19200, 38400, 57600, 115200 (default), 230400, 460800, 500000, 576000, and 921600.
To override, set the environment variable `ENERGYMON_OSP3_BAUD` to the device's configured baud.
Note that not all platforms support configuring all baud rates, e.g., macOS is limited.

There are three distinct power sources that can be monitored: channel 0 (default), channel 1, and the input power.
To configure the power source, set the environment variable `ENERGYMON_OSP3_POWER_SOURCE` to either `0`, `1`, or `IN`.

OSP3 devices can be configured to refresh anywhere between every 5 ms and 1 sec, depending on the configured baud.
The `osp3` implementation will automatically handle different update intervals.


## Linking

To link with the library and its dependencies, use `pkg-config` to get the linker flags:

```sh
pkg-config --libs --static energymon-osp3
```

The `--static` flag is unnecessary when using dynamically linked libraries.
