This project is under active development.
Things are likely to break and APIs are subject to change without warning.
Use at your own risk!

# Energy Monitoring Interface

EnergyMon provides a general C interface for energy monitoring utilities.

Applications using some libraries may need to be executed using elevated
privileges.

## Building

To build the libraries with the default dummy implementation as
`libenergymon.so`, run:

``` sh
make
```

To use a different implementation as `libenergymon-default.so`, e.g. the Intel
MSR energy monitor, run:

``` sh
make DEFAULT=msr
```

Current implementation options are:

* dummy [default]
* msr
* odroid
* osp
* osp-polling

You may cleanup builds with:

``` sh
make clean
```

## Installing

Installation places libraries in `/usr/local/lib` and header files in
`/usr/local/include/energymon`.

To install all libraries, run:

``` sh
sudo make install
```

## Uninstalling

To remove libraries and headers installed to the system, run:

``` sh
sudo make uninstall
```
