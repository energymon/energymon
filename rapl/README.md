# RAPL Energy Monitor

This implementation of the `energymon` interface reads from Intel Running
Average Power Limit (RAPL) sysfs files on Linux platforms.

RAPL zones can be found in the `/sys/class/powercap` directory.

Energy readings are made at top level domains, e.g. `intel-rapl:0` which
include energy data from subdomains, e.g. `intel-rapl:0:[0-2]`.
This means that energy readings include package (core and uncore) components
and DRAM.

## Prerequisites

You must be using a system that supports RAPL.

The `intel_rapl` kernel module must be loaded:

```sh
sudo modprobe intel_rapl
```

## Usage

To link with the shared object library:

```
-lenergymon-rapl
```

To link with the static library version:

```
-lenergymon-rapl-static
```
