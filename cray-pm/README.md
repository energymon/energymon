# Cray Power Monitoring

These implementations of the `energymon` interface read from Cray Power Monitoring sysfs interface.
For convenience, we combine the implementations into one library.

This library is designed for and tested on [NERSC Cori](http://www.nersc.gov/users/computational-systems/cori/), a Cray XC40 system.
Some energy counters are available on XC30 models.
Support for future systems is dependent on Cray maintaining the sysfs interface.

Specifically, these implementations support reading the following files in `/sys/cray/pm_counters` on at least the following system models:

* `energy` - XC30 and XC40 systems.
* `accel_energy` - XC30 and XC40 systems with accelerators.
* `cpu_energy` - XC40 systems with Intel KNL processors.
* `memory_energy` - XC40 systems with Intel KNL processors.

## Prerequisites

You must be running on a Cray system that supports the power monitoring interfaces listed above.

The `perftools-base` module must be loaded:

```sh
module load perftools-base
```

## Usage

You **must** specify a comma-delimited list of file names in the `/sys/cray/pm_counters` directory in the `ENERGYMON_CRAY_PM_COUNTERS` environment variable so the library knows which file(s) to use.
Supported values are listed above.
Some examples:

```sh
export ENERGYMON_CRAY_PM_COUNTERS=energy
export ENERGYMON_CRAY_PM_COUNTERS=energy,accel_energy
export ENERGYMON_CRAY_PM_COUNTERS=cpu_energy
export ENERGYMON_CRAY_PM_COUNTERS=cpu_energy,memory_energy
```

The implementation will sum the values from each file specified into a total energy value during reading.

## Linking

To link with the library:

```
-lenergymon-cray-pm
```
