# IBM PowerNV Energy Monitor

This implementation of the `energymon` interface is for IBM PowerNV systems.
It is developed primarily for use on [OLCF Summit](https://www.olcf.ornl.gov/summit/).
Power/energy data is exposed to userspace by the [ibmpowernv](https://www.kernel.org/doc/html/latest/hwmon/ibmpowernv.html) Linux kernel module.

## Prerequisites

This implementation depends on [libsensors](https://github.com/lm-sensors/lm-sensors).
On Ubuntu, install `libsensors4-dev`; on Red Hat based distros, install `lm_sensors-devel`.

## Linking

To link with the library and its dependencies, use `pkg-config` to get the linker flags:

```sh
pkg-config --libs --static energymon-ibmpowernv-power
```

The `--static` flag is unnecessary when using dynamically linked libraries.
