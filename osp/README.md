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

## Linking

To link with the library:

```
-lenergymon-osp -lhidapi-libusb
```

To link with the polling version of the library:

```
-lenergymon-osp-polling -lhidapi-libusb -lpthread
```

For the polling version, also need `-lrt` for glibc versions before 2.17.
