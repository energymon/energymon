# Energy Monitoring Interface

EnergyMon provides a general C interface for energy monitoring utilities.

For details, please see the following publication and reference as appropriate:

* Connor Imes, Lars Bergstrom, and Henry Hoffmann. "A Portable Interface for Runtime Energy Monitoring". In: FSE. 2016. DOI: https://doi.org/10.1145/2950290.2983956
* You may also find an extended analysis in the [Tech Report](https://cs.uchicago.edu/research/publications/techreports/TR-2016-08).

Applications using some libraries may need to be executed using elevated privileges.

The following instructions are for Linux systems.
If you are using a different platform, change the commands accordingly.

Current EnergyMon implementation options are:

* dummy [default]
* cray-pm
* msr
* odroid
* odroid-ioctl
* osp
* osp-polling
* rapl
* shmem
* wattsup
* wattsup-libusb
* wattsup-libftdi

## Building

This project uses CMake.

To build the libraries with the dummy implementation as the default, run:

``` sh
mkdir _build
cd _build
cmake ..
make
```

To use a different default implementation, e.g. the RAPL energy monitor, change the `cmake` command to specify `DEFAULT`:

``` sh
cmake -DDEFAULT=rapl ..
```

To build shared objects / dynamically linked libraries instead of static libraries, set `BUILD_SHARED_LIBS` when running `cmake`:

``` sh
cmake .. -DBUILD_SHARED_LIBS=ON
```

For an optimized build, set `CMAKE_BUILD_TYPE` when running `cmake`, e.g. one of:

``` sh
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
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

## Linking

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

## Usage

To use an EnergyMon implementation, you must first populate the struct by calling the getter function, then initialize it.
Don't forget to cleanup the instance once you're finished with it.
See `energymon.h` and `energymon-default.h` for more detailed function descriptions.

```C
  energymon em;
  uint64_t start_uj, end_uj;

  // get the energymon instance and initialize
  energymon_get_default(&em);
  em.finit(&em);

  // profile application function
  start_uj = em.fread(&em);
  do_work();
  end_uj = em.fread(&em);
  printf("Total energy for do_work() in microjoules: %"PRIu64"\n", end_uj - start_uj);

  // destroy the instance
  em.ffinish(&em);
```

## Tools

This project includes a handful of applications.

### Utilities

All of the following are linked with `energymon-default`:

* `energymon-cmd-profile`: Prints out time, energy, and power statistics for the execution of a given shell command.
* `energymon-power-poller`: Prints average power values at the requested interval for the previous interval period.
* `energymon-file-provider`: Writes energy data to a file (overwrites previous values).
* `energymon-idle-power`: Prints the average power over the given interval (meant to run in isolation and measure idle power consumption).
* `energymon-info`: Prints information about the implementation.
* `energymon-overhead`: Prints the latency overhead in nanoseconds of the functions `finit`, `fread`, and `ffinish`.

### Shared Memory Providers

Shared memory providers allow exposing energy data to one or more unprivileged applications for sources that require elevated privileges and/or exclusive access.
The providers may need to run with elevated privileges, but other applications can attach to their shared memory and read energy data using the `shmem` EnergyMon implementation.

* `energymon-osp-polling-shmem-provider`
* `energymon-wattsup-shmem-provider`

## Project Source

Find this and related project sources at the [energymon organization on GitHub](https://github.com/energymon).  
This project originates at: https://github.com/energymon/energymon


Bug reports and pull requests for new implementations, bug fixes, and enhancements are welcome.
