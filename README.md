# Energy Monitoring Interface

EnergyMon provides a general C interface for energy monitoring utilities.

Applications using some libraries may need to be executed using elevated
privileges.

## Building

To build the libraries with the dummy implementation as the default, run:

``` sh
make
```

To use a different default implementation, e.g. the RAPL energy monitor, run:

``` sh
make DEFAULT=rapl
```

Current implementation options are:

* dummy [default]
* msr
* odroid
* osp
* osp-polling
* rapl

You may cleanup builds with:

``` sh
make clean
```

## Installing

To install all libraries, headers, and binaries, run:

``` sh
sudo make install
```

On Linux, the installation usually places
libraries in `/usr/local/lib`,
header files in `/usr/local/include/energymon`, and
binary files in `/usr/local/bin`.

## Uninstalling

To remove files installed to the system, run:

``` sh
sudo make uninstall
```

## Usage

For instructions on linking with particular libraries, see the README files in
the appropriate subprojects.
