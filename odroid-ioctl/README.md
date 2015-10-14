# ODROID Energy Monitor via ioctl

This implementation of the `energymon` interface reads from INA-231 power
sensors on Hardkernel ODROID systems running Linux using the ioctl interface.
The power sensors are polled at regular intervals to estimate energy
consumption.

## Prerequisites

You must be using an ODROID system with embedded power sensors.
This has been tested on the XU+E and XU3 models.

## Usage

To link with the shared object library:

```
-lenergymon-odroid-ioctl -lpthread
```

To link with the static library version:

```
-lenergymon-odroid-ioctl-static -lpthread
```
