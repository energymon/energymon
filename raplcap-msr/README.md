# RAPLCap MSR Energy Monitor

This implementation of the `energymon` interface wraps the [raplcap-msr](https://github.com/powercap/raplcap) library.
This provides broader support for Intel processors than the [energymon-msr](../msr) implementation,
including automatic socket discovery.


## Prerequisites

First, you must be using a system that supports Intel MSRs.

Install [raplcap-msr](https://github.com/powercap/raplcap), including development header files.

See the raplcap-msr documentation for more information on prerequisites and configuring MSRs.


## Usage

By default, the `PACKAGE` RAPL zone will be used.
To force a particular zone, set the `ENERGYMON_RAPLCAP_MSR_ZONE` environment variable:

* `PACKAGE`
* `CORE`
* `UNCORE`
* `DRAM`
* `PSYS` or `PLATFORM`

## Linking

To link with the library:

```
pkg-config --libs --static energymon-raplcap-msr
```
