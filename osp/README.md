# ODROID Smart Power Energy Monitor

This implementation of the `energymon` interface reads from an ODROID Smart
Power USB device.

Two different library implementations are created.
The first reads energy data direct from the device on-demand.
The second polls the device for power data to estimate energy consumption.
Developer tests have found the polling implementation to be more reliable.

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
