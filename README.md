# Energy Monitoring Interface

EnergyMon provides a general C interface for energy monitoring utilities.

Applications using some libraries may need to be executed using elevated
privileges.

The following instructions are for Linux systems.
If you are using a different platform, change the commands accordingly.

Current EnergyMon implementation options are:

* dummy [default]
* msr
* odroid
* odroid-ioctl
* osp
* osp-polling
* rapl
* wattsup

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

To build static libraries instead of shared objects, turn off `BUILD_SHARED_LIBS` when running `cmake`:

``` sh
cmake .. -DBUILD_SHARED_LIBS=false
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

The best approach for linking with any EnergyMon library is to use [pkg-config](http://www.freedesktop.org/wiki/Software/pkg-config/).
This is especially useful if building and linking to static libraries to ensure that you link with transitive dependencies.

For example, to link with `energymon-default`, whose implementation is not always known in advance:

```
pkg-config energymon-default --libs --static
```

If your project is using `CMake`, you can use pkg-config to find the library and necessary flags.
For example, to find `energymon-default`, its headers, and to link with it and any transitive dependencies:

``` cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(ENERGYMON REQUIRED energymon-default)
include_directories(${ENERGYMON_INCLUDE_DIRS})

add_executable(hello_world hello_world.c)
target_link_libraries(hello_world -L${ENERGYMON_LIBDIR} ${ENERGYMON_LIBRARIES})
```
