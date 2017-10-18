# Release Notes

## Unreleased

 * Added VERSION and SOVERSION properties to shared object libraries
 * Only export symbols in shared object libraries that are declared in public headers (prevents symbol name clashes)
 * Use GNU standard installation directories (multiarch support)
 * Increased minimum CMake version from 2.8 to 2.8.5 to support GNUInstallDirs
 * Added man pages for utility applications; bug fixes and improvements
  * energymon-cmd-profile: Accept commands without quotations; add help option
  * energymon-file-provider: Add help, count, interval, and no-rewind options
  * energymon-idle-power: Add help option

## v0.2.1 - 2017-10-01

 * Added support for msr-safe kernel module in msr implementation
 * Added cray-pm implementations to support capabilities of Cray XC30 and XC40 systems
 * Added wattsup-libusb and wattsup-libftdi implementations
 * Added AppVeyor test configuration for Windows continuous integration
 * Fixed #37: In osp implementation, force overflow in ODROID Smart Power at 1000 Wh to avoid device counter saturation at 8192 Wh
 * Fixed #27: More modular build process
 * Deprecated custom DEFAULT CMake option in favor of ENERGYMON_BUILD_DEFAULT
 * Removed Android CMake toolchain file: cmake-toolchain/android.toolchain.cmake
 * Faster refresh in osp-polling, odroid and odroid-ioctl implementations
 * Better multi-platform support for some implementations, especially with portable time and sleep abstractions
 * Downgraded required CMake version from 2.8.9 to 2.8
 * Documented FSE 2016 and Technical Report publication information
 * Numerous bug fixes, enhancements, optimizations, and documentation improvements

## v0.2.0 - 2016-10-12

 * Initial public release
