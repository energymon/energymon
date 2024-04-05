# RAPL Energy Monitor

This implementation of the `energymon` interface reads from Intel Running
Average Power Limit (RAPL) sysfs files on Linux platforms (see the
[Linux Power Capping Framework](https://www.kernel.org/doc/html/latest/power/powercap/powercap.html)).

RAPL zones can be found in the `/sys/class/powercap` directory.

Specifically, this implementation reads the `energy_uj` file from RAPL
`package` domains, e.g., at `/sys/class/powercap/intel-rapl:0`.
Starting in Linux 5.10, reading this sysfs file requires root privileges.

No additional configuration is required for multi-package/die systems.
The interface returns the sum of energy values across packages/die.

## Prerequisites

You must be using a system that supports the `intel-rapl` powercap control type.

Ensure that the appropriate kernel module is loaded:

```sh
sudo modprobe intel_rapl_msr
```

On on kernels older than 5.3:

```sh
sudo modprobe intel_rapl
```
