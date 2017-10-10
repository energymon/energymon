# Release Notes

## Unreleased

 * Added VERSION and SOVERSION properties to shared object libraries

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
