This project is under active development.  
Things are likely to break and APIs are subject to change without warning.  
Use at your own risk!

# Energy Monitoring Interface

EnergyMon provides a general C interface for energy monitoring utilities.

Applications using some libraries may need to be executed using elevated
privileges.

## Building

To build the libraries and link `libenergymon.so` to the default dummy
implementation (`libenergymon-dummy.so`), run:

``` sh
make
```

To build with a different library linked from `libenergymon.so`, e.g. the
Intel MSR energy monitor, run:

``` sh
make IMPL=libenergymon-msr.so
```

## Installing

Installation places libraries in `/usr/local/lib` and header files in
`/usr/local/include/energymon`.

To install all libraries with the default library linked from
`libenergymon.so`, run:

``` sh
sudo make install
```

To install a different library, e.g. the Intel MSR energy monitor, as the
default, run:

``` sh
sudo make install IMPL=libenergymon-msr.so
```

## Uninstalling

To remove libraries and headers installed to the system, run:

``` sh
sudo make uninstall
```
