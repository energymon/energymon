# Watts up? Energy Monitor

These implementations of the `energymon` interface poll power from `Watts up?` power meters connected by USB.

There are three implementations that are mutually exclusive:

* `energymon-wattsup` - Uses the /dev filesystem on Linux and macOS.
* `energymon-wattsup-libusb` - Uses [libusb-1.0](http://www.libusb.org/wiki/libusb-1.0).
* `energymon-wattsup-libftdi` - Uses [libftdi](https://www.intra2net.com/en/developer/libftdi/) (WattsUp devices use a FTDI FT232R USB to serial UART IC internally).

If you're unsure which library to use, `energymon-wattsup-libftdi` is probably the most portable.

## Prerequisites

You need a `Watts up?` power meter with a USB connection.
This has been tested on a `Watts up? PRO ES` device.

To use `energymon-wattsup-libftdi`, `libftdi1` (and development headers) must be discoverable by `pkg-config`.
To use `energymon-wattsup-libusb`, `libusb-1.0` (and development headers) must be discoverable by `pkg-config`.

E.g., on Debian-based Linux systems, including Ubuntu:

```sh
sudo apt install libftdi1-dev libusb-1.0-0-dev
```

Or on macOS, using [Homebrew](https://brew.sh/):

```sh
brew install libftdi libusb
```

> NOTE: Support for the original `libftdi` (which requires the legacy `libusb-0.1`) is deprecated and will be removed in the future.

### Privileges

Typically, sudo/root privileges are needed to read and write from the USB device file, e.g. `/dev/ttyUSB0`.
To use `energymon-wattsup` without sudo/root at runtime, you can add your user to the file owner's group.
For example, if the device file is owned by `root:dialout` and the file permissions permit both reading and writing by the group owner (`dialout`), then add the current user to the `dialout` group:

```sh
sudo usermod -a -G dialout $USER
```

The user may need to logout and log back in for the permission changes to take effect.

To use the `libusb` or `libftdi` libraries without using sudo/root at runtime, set appropriate [udev](https://en.wikipedia.org/wiki/Udev) privileges.
For example, create a udev rule, e.g., append to `/etc/udev/rules.d/10-local.rules`:

```
# WattsUp? PRO USB device
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", GROUP="dialout"
```

The WattsUp device probably needs to be disconnected and reconnected, or the system rebooted, for the change to take effect (the device must be remounted by the kernel with the new permissions).

## Usage

`Watts up?` devices refresh about once per second.
The documentation specifies that the device will respond to requests within 2 seconds.
As a result, it's possible that energy data could be delayed by up to 2-3 seconds from the actual power behavior.

By default, the `wattsup` implementation looks for the WattsUp device at `/dev/ttyUSB0`.
To override, set the environment variable `ENERGYMON_WATTSUP_DEV_FILE` to the correct device file (e.g., a different device number like `/dev/ttyUSB1` on Linux or a "calling unit" device like `/dev/cu.usbserial-A5015A7F` on macOS).

The `wattsup-libusb` implementation detaches the WattsUp device from the kernel (thus unmounting it from the /dev filesystem in Linux during runtime), then reattaches it when giving up the device during teardown (but only if it was found to be attached during initialization).
The `wattsup-libftdi` implementation also detaches the kernel driver, but only (and unconditionally) reattaches it during teardown if `libftdi >= 1.5`; otherwise you can reattach the device to the kernel using the `energymon-wattsup-attach-kernel` binary (requires `libusb-1.0`).

## Linking

To link with the appropriate library and its dependencies, use `pkg-config` to get the linker flags:

```sh
pkg-config --libs --static energymon-wattsup
pkg-config --libs --static energymon-wattsup-libusb
pkg-config --libs --static energymon-wattsup-libftdi
```

The `--static` flag is unnecessary when using dynamically linked libraries.
