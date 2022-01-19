# NVIDIA Jetson Energy Monitor

This implementation of the `energymon` interface reads from TI INA3221 power monitors on NVIDIA Jetson systems running [Jetson Linux](https://developer.nvidia.com/embedded/linux-tegra).
The power monitors are polled at regular intervals to estimate energy consumption.

## Prerequisites

You must be using an NVIDIA Jetson system with the L4T INA3221 kernel driver.

Jetson models that should be supported:

* TX1
* Nano (original only - the Jetson Nano 2GB does not have INA3221 power monitors)
* Xavier NX
* AGX Xavier Series
* TX2, TX2i, TX2 4GB
* TX2 NX

## Usage

Different platforms support varying numbers of sensor devices, with each sensor supporting up to three "rails" or "channels".
By default, the system-level power rail will be used if it's found, e.g., `VDD_IN` (most platforms) or `5V_IN` (Xavier NX).
On the AGX Xavier Series, there's no system-level or parent rail, so all rails are used (they're in parallel to each other).

To override this behavior and read from different rails, set the environment variable `ENERGYMON_JETSON_RAIL_NAMES`.
See the "Clock Frequency and Power Management" section of the [NVIDIA Jetson Linux Driver Package (L4T)](https://docs.nvidia.com/jetson/l4t/) documentation for allowable values on each platform.
A comma-delimited list is supported to aggregate power readings from multiple rails, but use caution to avoid specifying overlapping hardware sources, otherwise power/energy will be counted more than once.

## Linking

To link with the appropriate library and its dependencies, use `pkg-config` to get the linker flags:

```sh
pkg-config --libs --static energymon-jetson
```

The `--static` flag is unnecessary when using dynamically linked libraries.
