# Energy Monitor over Shared Memory

This implementation of the `energymon` interface reads data from a shared
memory location.

## Usage

To use this implementation, a program must be providing energy data to a shared
memory location in the format of the `energymon_shmem` struct defined in
[energymon-shmem.h](energymon-shmem.h).
An example of how to do so is available in
[example/energymon-shmem-example.c](example/energymon-shmem-example.c).
The shared memory must be available before the `energymon` struct is initialized.

Both the provider and this library must agree on the `path` and `id` used to
create the IPC shared memory key, as required by the POSIX `ftok` function.
By default, this library uses the current working directory `"."` for the path
and the value `1` for the id.
The id must be between 0 and 255 as only the low-order 8 bits of the integer
are significant, per the function documentation.
The default values are overridden by setting the environment variables
`ENERGYMON_SHMEM_DIR` and `ENERGYMON_SHMEM_ID`, respectively.

## Linking

To link with the library:

```
-lenergymon-shmem
```
