# ODROID Smart Power 3

This implementation of the `energymon` interface reads from an [ODROID Smart Power 3](https://wiki.odroid.com/accessory/power_supply_battery/smartpower3) USB device.

> NOTE: If you're using an older generation [ODROID Smart Power](https://wiki.odroid.com/old_product/accessory/odroidsmartpower) device, use the `osp` or `osp-polling` implementation instead.


## Prerequisites

You need an ODROID Smart Power 3 device with a USB connection.

This implementation depends on [osp3](https://github.com/energymon/osp3/).


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
