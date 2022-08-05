# Jetson Power Rails

This document outlines INA3221 power sensor configurations on supported Jetson devices.
The following information is based on:

* [Jetson Developer Guide](https://docs.nvidia.com/jetson/l4t/), "Clock Frequency and Power Management" section. Version: 32.6.1 (Accessed 2022-01-13).
* [L4T kernel sources](https://developer.nvidia.com/embedded/linux-tegra), device trees in `hardware/nvidia/platform/`. Version: 32.6.1 (Accessed 2022-02-25).
* [Jetson Developer Guide](https://docs.nvidia.com/jetson/l4t/), "Platform Power and Performance" section. Version: 34.1 (Accessed 2022-08-05).
* Additional documents at the [Jetson Download Center](https://developer.nvidia.com/embedded/downloads).

Note: L4T <= 32.x (JetPack 4.x) starts channel numbering at 0; L4T >= 34.1 (JetPack 5.x) starts channel numbering at 1.


## TX1

L4T kernel sources refer to this model family as "jetson".

| I2C Address | Channel 0 | Channel 1 | Channel 2 |
| --- | --- | --- | --- |
| 0x40 | VDD_IN | VDD_GPU | VDD_CPU |
| 0x42 (carrier board) | VDD_MUX | VDD_5V_IO_SYS | VDD_3V3_SYS |
| 0x43\* (carrier board) | VDD_3V3_IO | VDD_1V8_IO | VDD_M2_IN |

\* 0x43: Table names based on kernel device tree source inspection (docs specify names: `VDD_3V3_IO_SLP`, `VDD_1V8_IO`, `VDD_3V3_SYS_M2`).

`VDD_IN` supplies the main module and `VDD_MUX` supplies the carrier board.
They appear to branch from the main power supply, so should be in parallel.


## Nano

Note: L4T kernel sources refer to this model family as "porg".

The Jetson Nano 2GB does not have an INA3221 power monitor.

| I2C Address | Channel 0 | Channel 1 | Channel 2 |
| --- | --- | --- | --- |
| 0x40\* | POM_5V_IN | POM_5V_GPU | POM_5V_CPU |

\* 0x40: Table names based on kernel device tree source inspection and real platform observations (docs specify names: `VDD_IN`, `VDD_GPU`, `VDD_CPU`).


## Xavier NX

L4T kernel sources refer to this model family as "jakku".

| I2C Address | Channel 0 | Channel 1 | Channel 2 |
| --- | --- | --- | --- |
| 0x40\* | VDD_IN | VDD_CPU_GPU_CV | VDD_SOC |

\* 0x40: Table names based on kernel device tree source inspection (docs specify names: `5V_IN`, `VDD_CPU_GPU`, `VDD_SOC`).


## AGX Xavier Series

L4T kernel sources refer to this model family as "galen".

| I2C Address | Channel 0 | Channel 1 | Channel 2 |
| --- | --- | --- | --- |
| 0x40 | GPU | CPU | SOC |
| 0x41 | CV | VDDRQ | SYS5V |

There doesn't seem to be a channel for a system/parent power rail.
From "Jetson AGX Xavier Series Product Design Guide" (Accessed 2021-01-18):
Section 5.6.2 states, "These monitor the VDD_GPU, VDD_CPU, VDD_SOC (Core), VDD_CV, VDD_DDRQ, and SYS_VIN_MV Supplies", and 
Section 5.2 states, "The SYS_VIN_HV and SYS_VIN_MV are derived from [the main] power source" (and not one from the other).
Per this text and Table 5-2, we infer that the `SYS_VIN_MV` rail supplies the `SYS5V` channel and the `SYS_VIN_HV` rail supplies the other channels.


## TX2, TX2i, and TX2 4GB

L4T kernel sources refer to this model family as "quill".

| I2C Address | Channel 0 | Channel 1 | Channel 2 |
| --- | --- | --- | --- |
| 0x40 | VDD_SYS_GPU | VDD_SYS_SOC | VDD_4V0_WIFI\*\* |
| 0x41 | VDD_IN | VDD_SYS_CPU | VDD_SYS_DDR |
| 0x42\* (carrier board) | VDD_MUX | VDD_5V0_IO_SYS | VDD_3V3_SYS |
| 0x43 (carrier board)| VDD_3V3_IO_SLP | VDD_1V8_IO | VDD_3V3_SYS_M2 |

\* 0x42: Table names based on kernel device tree source inspection (docs specify names: `VDD_MUX`, `VDD_5V_IO_SYS`, `VDD_3V3_SYS`).

\*\* `VDD_4V0_WIFI`: Not available on the TX2i.

`VDD_IN` supplies the main module and `VDD_MUX` supplies the carrier board.
They appear to branch from the main power supply, so should be in parallel.
See: [TX2 power rail relationships](https://forums.developer.nvidia.com/t/tx2-power-rail-relationships/201037).


## TX2 NX

L4T kernel sources refer to this model family as "lanai".

| I2C Address | Channel 0 | Channel 1 | Channel 2 |
| --- | --- | --- | --- |
| 0x40 | VDD_IN | VDD_CPU_GPU | VDD_SOC |


## AGX Orin Series

| I2C Address | Channel 1 | Channel 2 | Channel 3 |
| --- | --- | --- | --- |
| 0x40 | VDD_GPU_SOC | VDD_CPU_CV | VIN_SYS_5V0 |
| 0x41 | NC\* | VDDQ_VDD2_1V8AO | NC\* |

\* Per the Developer Guide, `NC` means "No Connection".

The `SYS_VIN_HV` power source supplies the `CPU`, `GPU`, `SOC`, and `CV`; the `SYS_VIN_MV` power source supplies `VIN_SYS_5V0`; `VIN_SYS_5V0` in turn supplies `VDDQ_VDD2_1V8AO`.
See: [Jetson AGX Orin INA3221 power monitor layout](https://forums.developer.nvidia.com/t/jetson-agx-orin-ina3221-power-monitor-layout/223111)
