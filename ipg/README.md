# Intel&reg; Power Gadget Energy Monitor

This implementation of the `energymon` interface uses [Intel&reg; Power Gadget](https://www.intel.com/content/www/us/en/developer/articles/tool/power-gadget.html).


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


## Linking

To link with the library:

```
pkg-config --libs --static energymon-ipg
```
