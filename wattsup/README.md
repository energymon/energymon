# Watts up? Energy Monitor

This implementation of the `energymon` interface polls power from `Watts up?`
power meters connected by USB in Linux systems.

## Prerequisites

You need a `Watts up?` power meter with a USB connection.
This has been tested on a `Watts up? PRO ES` device.

## Usage

`Watts up?` devices refresh about once per second.
The documentation specifies that the device will respond to requests within 2
seconds.
As a result, it's possible that energy data could be delayed by up to 2-3
seconds from the actual power behavior.

By default, the implementation looks for the WattsUp device at `/dev/ttyUSB0`.
To override, set the environment variable `ENERGYMON_WATTSUP_DEV_FILE` to the
correct device file.

## Linking

To link with the library:

```
-lenergymon-wattsup -lpthread
```

Also need `-lrt` for glibc versions before 2.17.
