# MSR Energy Monitor

This implementation of the `energymon` interface reads Package-level energy
data from Intel Model Specific Registers on Linux platforms.

## Prerequisites

First, you must be using a system that supports Intel MSRs.

The implementation first checks for the [msr-safe](https://github.com/LLNL/msr-safe)
kernel module, otherwise it falls back on the `msr` kernel module.

To load the `msr` kernel module:

```sh
sudo modprobe msr
```

### Privileges

Typically, sudo/root privileges are needed to read from MSR device files
which are located at `/dev/cpu/*/msr`.
Instead of launching an application that uses this interface with root
privileges, a user may be able to `chmod` the `msr` file(s) to
world-readable, e.g.:

```sh
sudo chmod og+r /dev/cpu/0/msr
```

On some Linux kernels you may still get an `Operation not permitted` error.
Set RAWIO permissions on your executable:

```sh
sudo setcap cap_sys_rawio+ep BINARY
```

If using the `msr-safe` module, ensure that your user has read privileges to
the `msr_safe` file(s), like above, and that the following registers have read
permissions enabled:

* `MSR_RAPL_POWER_UNIT`
* `MSR_PKG_ENERGY_STATUS`

You can add them to the whitelist by running from this directory:

```sh
sudo sh -c 'cat etc/msr_safe_whitelist >> /dev/cpu/msr_whitelist'
```

## Usage

By default, only the MSR for cpu 0 will be accessed.
To override this behavior, you can configure the MSRs to access by setting the
`ENERGYMON_MSRS` environment variable with a comma-delimited list of CPUs to
read from, e.g.:

```sh
export ENERGYMON_MSRS=0,4,8,12
```

## Linking

To link with the library:

```
-lenergymon-msr
```
