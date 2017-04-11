# ODROID Smart Power Energy Monitor

These implementations of the `energymon` interface read from an ODROID Smart
Power USB device.

Two different library implementations are available.
The first reads energy data directly from the device.
The second polls the device for power data to estimate energy consumption.

The first implementation converts energy data from Watt-Hours at 2 decimal
places of precision, which is a little imprecise in low-power scenarios.
However, it has less runtime overhead than the polling implementation.

## Prerequisites

You need an ODROID Smart Power device with a USB connection.

This implementation depends on [hidapi](https://github.com/signal11/hidapi/).
On Ubuntu 14.04 LTS and newer, just install `libhidapi-dev`.

### Linux Privileges

To use an ODROID Smart Power without needing sudo/root at runtime, set appropriate [udev](https://en.wikipedia.org/wiki/Udev) privileges.

For example, you can give access to a specific group, e.g. `plugdev`, by creating/modifying a `udev` config file, e.g. `/etc/udev/rules.d/10-local.rules`.
Depending on whether you are using the `libusb` or `hidraw` implementations of `hidapi`, use one of the following rules (though having both shouldn't hurt):

```sh
# ODROID Smart Power - HIDAPI/libusb
SUBSYSTEM=="usb", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="003f", GROUP="plugdev"

# ODROID Smart Power - HIDAPI/hidraw
KERNEL=="hidraw*", ATTRS{busnum}=="1", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="003f", GROUP="plugdev"
```

The device probably needs to be disconnected and reconnected, or the system rebooted, for the change to take effect (the device must be remounted by the kernel with the new permissions).

## Linking

To link with the appropriate library and its dependencies, use `pkg-config` to get the linker flags:

```sh
pkg-config --libs --static energymon-osp
pkg-config --libs --static energymon-osp-polling
```

The `--static` flag is unnecessary when using dynamically linked libraries.
