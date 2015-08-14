# MSR Energy Monitor

This implementation of the `energymon` interface reads from Intel Model
Specific Registers on Linux platforms.

## Prerequisites

First, you must be using a system that supports MSRs.

To access MSRs, the `msr` kernel module needs to be loaded:

```sh
sudo modprobe msr
```

Typically, sudo/root privileges are needed to read from MSR device files
which are located at `/dev/cpu/*/msr`.
Instead of launching an application that uses this interface with root
privileges, a user may be able to `chmod` the `msr` file(s) to
world-readable, e.g.:

```sh
sudo chmod og+r /dev/cpu/0/msr
```

## Usage

By default, only the MSR for cpu 0 will be accessed.
To override this behavior, you can configure the MSRs to access by setting the
`ENERGYMON_MSRS` environment variable with a comma-delimited list of CPUs to
read from, e.g.:

```sh
export ENERGYMON_MSRS=0,4,8,12
```

To link with the shared object library:

```
-lenergymon-msr -lm
```

To link with the static library version:

```
-lenergymon-msr-static -lm
```
