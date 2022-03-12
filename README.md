# Energy Monitoring Interface

EnergyMon provides a general C interface for energy monitoring utilities.

If using this project for other scientific works or publications, please reference:

* Connor Imes, Lars Bergstrom, and Henry Hoffmann. "A Portable Interface for Runtime Energy Monitoring". In: FSE. 2016. DOI: https://doi.org/10.1145/2950290.2983956
  <details>
  <summary>[BibTex]</summary>

  ```BibTex
  @inproceedings{imes2016energymon,
    author = {Imes, Connor and Bergstrom, Lars and Hoffmann, Henry},
    title = {A Portable Interface for Runtime Energy Monitoring},
    year = {2016},
    isbn = {9781450342186},
    publisher = {Association for Computing Machinery},
    address = {New York, NY, USA},
    url = {https://doi.org/10.1145/2950290.2983956},
    doi = {10.1145/2950290.2983956},
    booktitle = {Proceedings of the 2016 24th ACM SIGSOFT International Symposium on Foundations of Software Engineering},
    pages = {968–974},
    numpages = {7},
    keywords = {portable energy measurement},
    location = {Seattle, WA, USA},
    series = {FSE 2016}
  }
  ```

  </details>
* You may also find an extended analysis in the [Tech Report](https://cs.uchicago.edu/research/publications/techreports/TR-2016-08).
  <details>
  <summary>[BibTex]</summary>

  ```BibTex
  @techreport{imes2016energymon-tr,
    author = {Imes, Connor and Bergstrom, Lars and Hoffmann, Henry},
    title = {A Portable Interface for Runtime Energy Monitoring: Extended Analysis},
    institution = {University of Chicago, Department of Computer Science},
    number = {TR-2016-08},
    year = {2016},
    month = sep,
    address = {Chicago, IL, USA}
  }
  ```

  </details>

Applications using some libraries may need to be executed using elevated privileges.

The following instructions are for Linux systems.
If you are using a different platform, change the commands accordingly.

Current EnergyMon implementation options are:

* **dummy** [default]: Mock implementation
* **cray-pm**: Cray XC30 and XC40 systems (e.g., NERSC Cori) via Linux sysfs files
* **ibmpowernv**: IBM PowerNV systems (e.g., OLCF Summit) via Linux sysfs energy sensor files
* **ibmpowernv-power**: IBM PowerNV systems (e.g., OLCF Summit) via Linux sysfs power sensor files
* **ipg**: Intel RAPL via `Intel Power Gadget`
* **jetson**: NVIDIA Jetson systems with INA3221 power sensors via Linux sysfs files
* **msr**: Intel RAPL via Linux Model-Specific Register device files (supports most non-Atom CPUs)
* **odroid**: Hardkernel ODROID XU+E and XU3 systems (with INA-231 power sensors) via Linux sysfs files
* **odroid-ioctl**: Hardkernel ODROID XU+E and XU3 systems (with INA-231 power sensors) via `ioctl` on Linux device files
* **osp**: Hardkernel ODROID Smart Power meters (coarse-grained energy counter) via `HIDAPI`
* **osp-polling**: Hardkernel ODROID Smart Power meters (finer-grained power sensor) via `HIDAPI`
* **rapl**: Intel RAPL via Linux powercap sysfs files
* **raplcap-msr**: Intel RAPL via `libraplcap-msr` (more capable than `msr` implementation above)
* **shmem**: Shared memory client via an EnergyMon shared memory provider
* **wattsup**: Watts Up Pro meter via Linux device files
* **wattsup-libusb**: Watts Up Pro meter via `libusb`
* **wattsup-libftdi**: Watts Up Pro meter via `libftdi`
* **zcu102**: Xilinx Zynq UltraScale+ ZCU102 systems (with INA-226 power sensors) via Linux sysfs files

See the README files in subdirectories for implementation specifics, including dependencies.


## Building

This project uses CMake.

By default, all libraries will be built, with `dummy` as the `energymon-default` implementation:

``` sh
mkdir _build
cd _build
cmake ..
make
```

To use a different default implementation, e.g., the RAPL energy monitor, specify `ENERGYMON_BUILD_DEFAULT` with cmake:

``` sh
cmake -DENERGYMON_BUILD_DEFAULT=rapl ..
```

Set `ENERGYMON_BUILD_DEFAULT=NONE` to disable building a default implementation.
Its default value is `dummy`.

To build only a single library, e.g., the RAPL energy monitor, specify `ENERGYMON_BUILD_LIB` with cmake:

``` sh
cmake -DENERGYMON_BUILD_LIB=rapl ..
```

Set `ENERGYMON_BUILD_LIB=NONE` to only build the default implementation (if set).
Set back to its default value of `ALL` to build all libraries.

To build shared objects / dynamically linked libraries instead of static libraries, set `BUILD_SHARED_LIBS` with cmake:

``` sh
cmake .. -DBUILD_SHARED_LIBS=ON
```

For an optimized build, set `CMAKE_BUILD_TYPE` when with cmake, e.g., one of:

``` sh
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

Of course, you can specify multiple options, e.g.:

``` sh
cmake .. -DENERGYMON_BUILD_LIB=NONE -DENERGYMON_BUILD_DEFAULT=rapl -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
```

### Other Build Options

Boolean options:

 * `ENERGYMON_BUILD_SHMEM_PROVIDERS` - enable/disable building shared memory providers (True by default)
 * `ENERGYMON_BUILD_UTILITIES` - enable/disable building utility applications (True by default)
 * `ENERGYMON_BUILD_TESTS` - enable/disable building test code (True by default)
 * `ENERGYMON_BUILD_EXAMPLES` - enable/disable building examples (True by default)


## Installing

To install libraries, headers, and binaries, run with proper privileges:

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

### CMake

Projects that use `CMake` can find the `EnergyMon` package and link against imported targets, which automatically applies properties like header search paths and transitive dependencies:

```cmake
find_package(EnergyMon REQUIRED)

add_executable(hello_world hello_world.c)
target_link_libraries(hello_world PRIVATE EnergyMon::energymon-default)
```

### pkg-config

A more general approach for linking with an EnergyMon library is to use [pkg-config](http://www.freedesktop.org/wiki/Software/pkg-config/):

```sh
cc $(pkg-config energymon-default --cflags) hello_world.c \
   $(pkg-config energymon-default --libs --static) -o hello_world
```

or in a Makefile:

```Makefile
CFLAGS += $(shell pkg-config --cflags energymon-default)
LDFLAGS += $(shell pkg-config --libs --static energymon-default)

hello_world: hello_world.c
	$(CC) $(CFLAGS) hello_world.c -o $@ $(LDFLAGS)
```

If shared object libraries are installed, don't include the `--static` option.

Projects that use `CMake >= 3.6` can alternatively use a pkg-config `IMPORTED_TARGET`, rather than the direct CMake imports documented above:

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(EnergyMonDefault REQUIRED IMPORTED_TARGET energymon-default)

add_executable(hello_world hello_world.c)
target_link_libraries(hello_world PRIVATE PkgConfig::EnergyMonDefault)
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

Implementation-specific versions of these utilities are also provided.

### Shared Memory Providers

Shared memory providers allow exposing energy data to one or more unprivileged applications for sources that require elevated privileges and/or exclusive access.
The providers may need to run with elevated privileges, but other applications can attach to their shared memory and read energy data using the `shmem` EnergyMon implementation.

* `energymon-osp-polling-shmem-provider`
* `energymon-wattsup-shmem-provider`
* `energymon-wattsup-libftdi-provider`
* `energymon-wattsup-libusb-provider`


## Project Source

Find this and related project sources at the [energymon organization on GitHub](https://github.com/energymon).  
This project originates at: https://github.com/energymon/energymon


Bug reports and pull requests for new implementations, bug fixes, and enhancements are welcome.
