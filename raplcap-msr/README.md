# RAPLCap MSR Energy Monitor

This implementation of the `energymon` interface wraps the [raplcap-msr](https://github.com/powercap/raplcap) library.
This provides broader support for Intel processors than the [energymon-msr](../msr) implementation,
including automatic RAPL instance (e.g., socket) discovery.


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

By default, all available RAPL instances will be used.
To force only particular instances, set the `ENERGYMON_RAPLCAP_MSR_INSTANCES` environment variable with a
comma-delimited list of IDs.
E.g., on a quad-socket system, to use only instances (sockets) 0 and 2 (and ignore 1 and 3):

```sh
export ENERGYMON_RAPLCAP_MSR_INSTANCES=0,2
```


## Linking

To link with the library:

```
pkg-config --libs --static energymon-raplcap-msr
```
