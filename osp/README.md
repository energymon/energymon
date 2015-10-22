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

The library `libhidapi-libusb` must be installed on the system to compile and
run these libraries.

## Usage

To link with the shared object library:

```
-lenergymon-osp -lhidapi-libusb
```

To link with the static library version:

```
-lenergymon-osp-static -lhidapi-libusb
```

To link with the shared object polling version of the library:

```
-lenergymon-osp-polling -lhidapi-libusb -lpthread
```

To link with the static polling version of the library:

```
-lenergymon-osp-polling-static -lhidapi-libusb -lpthread
```
