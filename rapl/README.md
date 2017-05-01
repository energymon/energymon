# RAPL Energy Monitor

This implementation of the `energymon` interface reads from Intel Running
Average Power Limit (RAPL) sysfs files on Linux platforms (see the
[Linux Power Capping Framework](https://www.kernel.org/doc/Documentation/power/powercap/powercap.txt)).

RAPL zones can be found in the `/sys/class/powercap` directory.

Specifically, this implementation reads the `energy_uj` file from RAPL
`package` domains, e.g. at `/sys/class/powercap/intel-rapl:0`.

No additional configuration is required for multi-socket systems.
The interface returns the sum of energy values across sockets.

## Prerequisites

You must be using a system that supports RAPL.

The `intel_rapl` kernel module must be loaded:

```sh
sudo modprobe intel_rapl
```

## Linking

To link with the library:

```
-lenergymon-rapl
```
