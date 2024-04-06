# Intel&reg; Power Gadget Energy Monitor

This implementation of the `energymon` interface uses [Intel&reg; Power Gadget](https://www.intel.com/content/www/us/en/developer/articles/tool/power-gadget.html).

> NOTE: Intel ended support for Power Gadget in October 2023 and no longer makes it available for download.
> Starting in EnergyMon v0.7.0, the `energymon-ipg` library is considered deprecated and may be removed in a future release.


## Prerequisites

* [Intel&reg; Power Gadget >= 3.7](https://www.intel.com/content/www/us/en/developer/articles/tool/power-gadget.html)

On OSX, install IPG using:
```sh
brew install intel-power-gadget
```


## Usage

By default, the `PACKAGE` RAPL zone will be used.
To use a different zone, set the `ENERGYMON_IPG_ZONE` environment variable:

* `PACKAGE`
* `CORE` or `IA`
* `DRAM`
* `PSYS` or `PLATFORM`
