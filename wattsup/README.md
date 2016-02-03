# WattsUp? Energy Monitor

This implementation of the `energymon` interface polls power from WattsUp?
Power meters connected by USB in Linux systems.

## Usage

WattsUp devices refresh about once per second.
To try and provide more accurate energy data, this implementation provides an
energy estimate when energy is read based on the elapsed time since the last
power reading from the device.
This means that energy data is actually based on power readings from up to one
second earlier.
To disable this behavior and only update energy at the device refresh interval,
set the environment variable `ENERGYMON_WATTSUP_DISABLE_ESTIMATES`.

By default, the implementation looks for the WattsUp device at `/dev/ttyUSB0`.
To override, set the environment variable `ENERGYMON_WATTSUP_DEV_FILE` to the
correct device file.

## Linking

To link with the library:

```
-lenergymon-wattsup -lpthread
```

Also need `-lrt` for glibc versions before 2.17.
