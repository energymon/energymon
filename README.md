# Energy Monitoring Interface

EnergyMon provides a general C interface for energy monitoring utilities.

Applications using some libraries may need to be executed using elevated
privileges.

The following instructions are for Linux systems.
If you are using a different platform, change the commands accordingly.

## Building

This project uses CMake.

To build the libraries with the dummy implementation as the default, run:

``` sh
mkdir _build
cd _build
cmake ..
make
```

To use a different default implementation, e.g. the RAPL energy monitor, change
the `cmake` command to specify `DEFAULT`:

``` sh
cmake -DDEFAULT=rapl ..
```

Current implementation options are:

* dummy [default]
* msr
* odroid
* odroid-ioctl
* osp
* osp-polling
* rapl

You may cleanup builds with:

``` sh
make clean
```

## Installing

To install all libraries, headers, and binaries, run with proper privileges:

``` sh
make install
```

On Linux, the installation usually places
libraries in `/usr/local/lib`,
header files in `/usr/local/include/energymon`, and
binary files in `/usr/local/bin`.

## Uninstalling

To remove files installed to the system, run with proper privileges:

``` sh
make uninstall
```

## Usage

For instructions on linking with particular libraries, see the README files in
the appropriate subprojects.
