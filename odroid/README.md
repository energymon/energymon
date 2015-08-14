# ODROID Energy Monitor

This implementation of the `energymon` interface reads from INA-231 power
sensors on Hardkernel ODROID systems running Linux.
The power sensors are polled at regular intervals to estimate energy
consumption.

## Prerequisites

You must be using an ODROID system with embedded power sensors.
This has been tested on the XU+E and XU3 models.

Sensors can be found in `/sys/bus/i2c/drivers/INA231/`.
To use the sensors, they must be enabled, e.g.:

```sh
echo 1 > /sys/bus/i2c/drivers/INA231/3-0040/enable
echo 1 > /sys/bus/i2c/drivers/INA231/3-0041/enable
echo 1 > /sys/bus/i2c/drivers/INA231/3-0044/enable
echo 1 > /sys/bus/i2c/drivers/INA231/3-0045/enable
```

## Usage

To link with the shared object library:

```
-lenergymon-odroid -lpthread
```

To link with the static library version:

```
-lenergymon-odroid-static -lpthread
```
