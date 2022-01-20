# NVIDIA Jetson Energy Monitor

This implementation of the `energymon` interface reads from TI INA3221 power monitors on NVIDIA Jetson systems running [Jetson Linux](https://developer.nvidia.com/embedded/linux-tegra).
The power monitors are polled at regular intervals to estimate energy consumption.

By default, the library reports energy at the largest scope possible on each supported Jetson model.
The following lists the supported models and the power rail(s) used by default on each.
When more than one rail is specified, their power values are aggregated.

* TX1: `VDD_IN`, `VDD_MUX`
* Nano (original only - the Jetson Nano 2GB does not have INA3221 power monitors): `VDD_IN`
* Xavier NX: `5V_IN`
* AGX Xavier Series: `GPU`, `CPU`, `SOC`, `CV`, `VDDRQ`, `SYS5V`
* TX2, TX2i, TX2 4GB: `VDD_IN`, `VDD_MUX`
* TX2 NX: `VDD_IN`

## Prerequisites

You must be using an NVIDIA Jetson system listed above with the L4T INA3221 kernel driver.

**Forward compatibility is not guaranteed!**
Using a different (e.g., newer) model *might* still produce energy results, but the default rail(s) selected by the library might not cover the correct power scope for the unknown model.
I.e., the rails' scopes (1) may not be the largest possible (readings will be too small), or (2) may overlap (readings will be too large), or (3) may be combination of (1) and (2) where some sensors are missed while others overlap with each other.
Setting the `ENERGYMON_JETSON_RAIL_NAMES` environment variable to an appropriate value as described below *might* overcome this problem.

## Usage

Different platforms support varying numbers of sensor devices, with each sensor supporting up to three "rails" or "channels".

To override the default behavior described above and read from different rails, set the environment variable `ENERGYMON_JETSON_RAIL_NAMES`.
See the "Clock Frequency and Power Management" section of the [NVIDIA Jetson Linux Driver Package (L4T)](https://docs.nvidia.com/jetson/l4t/) documentation for allowable values on each platform.
A comma-delimited list is supported to aggregate power readings from multiple rails, but use caution to avoid specifying overlapping hardware sources, otherwise power/energy will be counted more than once.
Refer to your platform's Product Design Guide to check power subsystem allocations.

## Linking

To link with the appropriate library and its dependencies, use `pkg-config` to get the linker flags:

```sh
pkg-config --libs --static energymon-jetson
```

The `--static` flag is unnecessary when using dynamically linked libraries.
