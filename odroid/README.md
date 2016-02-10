# ODROID Energy Monitor

These implementations of the `energymon` interface read from INA-231 power
sensors on Hardkernel ODROID systems running Linux.
The power sensors are polled at regular intervals to estimate energy
consumption.
One implementation uses `sysfs` and the other uses `ioctl`.

## Prerequisites

You must be using an ODROID system with embedded power sensors.
This has been tested on the XU+E and XU3 models.

## Usage

If using the `sysfs` implementation, the sensors must be enabled in advance.
Sensors can be found in `/sys/bus/i2c/drivers/INA231/`.
So for example, to enable the sensors:

```sh
echo 1 > /sys/bus/i2c/drivers/INA231/3-0040/enable
echo 1 > /sys/bus/i2c/drivers/INA231/3-0041/enable
echo 1 > /sys/bus/i2c/drivers/INA231/3-0044/enable
echo 1 > /sys/bus/i2c/drivers/INA231/3-0045/enable
```

## Linking

To link with the `sysfs` implementation:

```
-lenergymon-odroid -lpthread
```

To link with the `ioctl` implementation:

```
-lenergymon-odroid-ioctl -lpthread
```

For both implementations, also need `-lrt` for glibc versions before 2.17.
