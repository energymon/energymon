# Release Notes

## [Unreleased]

### Added

* ibmpowernv: new implementation for IBM PowerNV systems (targeted for OLCF Summit)
* ipg: new implementation using the Intel Power Gadget 3.7 API (PowerGadgetLib.h)
* jetson: new implementation for NVIDIA Jetson systems
* energymon-file-provider: -F/--force option to force updates faster than the EnergyMon implementation claims
* energymon-info: -h/--help option to print help message
* energymon-overhead: -h/--help option to print help message
* energymon-power-poller: -F/--force option to force updates faster than the EnergyMon implementation claims
* shmem: man pages for shared memory providers

### Changed

* Internal tweaks based on testing with additional compiler flags

### Fixed

* odroid, odroid-ioctl, zcu102: save errno when cleaning up after init failure
* osp: link with librt on older systems
* shmem: enforce provider ID in range `[1, 255]`
* wattsup-libftdi: compilation fails with clang when `-Werror` option is used
* utils: use implementation-specific binary names in help messages (don't assume energymon-default name format)


## [v0.4.0] - 2021-05-12

### Added

* ibmpowernv-power: new implementation for IBM PowerNV systems (targeted for OLCF Summit)
* raplcap-msr: support for multi-die packages
* Travis CI: test on additional architectures

### Changed

* CMake: overhaul the project build system
  * Increased minimum CMake version from 2.8.5 to 3.6
  * Refactor library and binary target creation
  * Export CMake targets during install
* rapl: document Linux kernel changes/differences in README
* raplcap-msr: update minimum dependency version to 0.5.0
* wattsup-libftdi [#47]: prefer `ftdi_tcioflush` if available (`ftdi_usb_purge_buffers` deprecated in libftdi 1.5)
* energymon-idle-power: now accepts idle time in fractional seconds

### Fixed

* Valid 0 return values from fread functions mistaken for errors when errno is already set
* cray-pm: building as default libraries used wrong macro, causing build failures
* osp: some cumulative energy unaccounted for during overflow detection (precision loss)
* raplcap-msr: Requires.private field in pkg-config file should specify the minimum version for raplcap-msr dependency
* wattsup-libftdi: Description field in pkg-config file was empty
* energymon-power-poller: incorrect Pmin summary output when number of samples is 0


## [v0.3.1] - 2020-10-27

### Fixed

* rapl [#46]: Check that top-level zones are actually package (not psys) zones
* rapl: Set errno when when no zones are found during init


## [v0.3.0] - 2020-03-10

### Added

* raplcap-msr: New implementation using libraplcap-msr (more features than `msr` and `rapl` implementations)
* utils [#44]: Build utilities for all implementations, not just energymon-default
* test: Build tests for all implementations, not just energymon-default

### Changed

* man pages: More pedantic formatting
* RELEASES: Reformat file

### Fixed

* cmake [#14]: Print useful CMake error when bad ENERGYMON_BUILD_DEFAULT value is set
* rapl: Powercap directory paths use lower case hex values, not decimal values
* zcu102: Potential memory leak in 'get_sensor_directories' under failure conditions


## [v0.2.4] - 2018-06-01

### Changed

* WattsUp implementations:
  * Wait up to 5 seconds for a good data packet (or fail) before completing initialization
  * Purge data buffers during initialization
  * Poll device at shorter intervals to increase energy value accuracy
  * Better handling of incomplete data packets
* osp-polling: Allow polling thread to be interrupted during sleep when finish is requested
* energymon-power-poller: Account for true elapsed time to produce more accurate power values

### Fixed

* wattsup-libftdi, wattsup-libusb: Fix empty reads by always setting baud rate and serial line properties
* wattsup-dev: Use pselect to avoid timer expired errors
* odroid, odroid-ioctl, osp-polling, zcu102: Minor correction to polling delay interval
* msr: Possible (but unlikely) precision computation bug
* osp, osp-polling: Fix status check during init that didn't always work


## [v0.2.3] - 2017-11-19

### Added

* Implementation for Xilinx ZCU102 (Jason Miller)

### Fixed

* Fixed [#40]: Time conversion bug in common function 'energymon_gettime_elapsed_us' (Jason Miller)


## [v0.2.2] - 2017-10-26

### Added

* New capabilities to some utility applications:
  * energymon-cmd-profile: Accept commands without quotations; add help option
  * energymon-file-provider: Add help, count, interval, and no-rewind options
  * energymon-idle-power: Add help option
* Man pages for utility applications
* VERSION and SOVERSION properties on shared object libraries
* Multiarch support (use GNU standard installation directories)

### Changed

* Increased minimum CMake version from 2.8 to 2.8.5 to support GNUInstallDirs
* Refactor this RELEASES.md file

### Removed

* Removed private symbol exports in shared object libraries (prevents symbol name clashes)

### Fixed

* Various bug fixes to utility applications


## [v0.2.1] - 2017-10-01

### Added

* Support for msr-safe kernel module in msr implementation
* New cray-pm implementations to support capabilities of Cray XC30 and XC40 systems
* New wattsup-libusb and wattsup-libftdi implementations
* AppVeyor test configuration for Windows continuous integration

### Changed

* Faster refresh in osp-polling, odroid and odroid-ioctl implementations
* Better multi-platform support for some implementations, especially with portable time and sleep abstractions
* Downgraded required CMake version from 2.8.9 to 2.8
* Documented FSE 2016 and Technical Report publication information
* Various enhancements, optimizations, and documentation improvements

### Deprecated

* Deprecated custom DEFAULT CMake option in favor of ENERGYMON_BUILD_DEFAULT

### Removed

* Removed Android CMake toolchain file: cmake-toolchain/android.toolchain.cmake

### Fixed

* Fixed [#37]: In osp implementation, force overflow in ODROID Smart Power at 1000 Wh to avoid device counter saturation at 8192 Wh
* Fixed [#27]: More modular build process
* Numerous other bugs

## v0.2.0 - 2016-10-12

### Added

* Initial public release


[Unreleased]: https://github.com/energymon/energymon/compare/v0.4.0...HEAD
[v0.4.0]: https://github.com/energymon/energymon/compare/v0.3.1...v0.4.0
[v0.3.1]: https://github.com/energymon/energymon/compare/v0.3.0...v0.3.1
[v0.3.0]: https://github.com/energymon/energymon/compare/v0.2.4...v0.3.0
[v0.2.4]: https://github.com/energymon/energymon/compare/v0.2.3...v0.2.4
[v0.2.3]: https://github.com/energymon/energymon/compare/v0.2.2...v0.2.3
[v0.2.2]: https://github.com/energymon/energymon/compare/v0.2.1...v0.2.2
[v0.2.1]: https://github.com/energymon/energymon/compare/v0.2.0...v0.2.1
[#47]: https://github.com/energymon/energymon/issues/47
[#46]: https://github.com/energymon/energymon/issues/46
[#44]: https://github.com/energymon/energymon/issues/44
[#40]: https://github.com/energymon/energymon/issues/40
[#37]: https://github.com/energymon/energymon/issues/37
[#27]: https://github.com/energymon/energymon/issues/27
[#14]: https://github.com/energymon/energymon/issues/14
