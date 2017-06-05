/**
 * Capture the time/energy behavior of a system command.
 *
 * @author Connor Imes
 * @date 2016-03-02
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "energymon-default.h"
#include "energymon-time-util.h"

int main(int argc, char** argv) {
  uint64_t time_start_ns;
  uint64_t time_end_ns;
  uint64_t time_total_ns;
  uint64_t energy_start_uj;
  uint64_t energy_end_uj;
  uint64_t energy_total_uj;
  double watts;
  char* cmd;
  int cmd_ret;
  energymon em;

  if (argc < 2) {
    fprintf(stderr, "Must supply system command as parameter.\n");
    exit(1);
  }
  cmd = argv[1];

  if (energymon_get_default(&em)) {
    perror("energymon_get_default");
    exit(1);
  }

  // initialize
  if (em.finit(&em)) {
    perror("energymon:finit");
    exit(1);
  }

  // get start time/energy
  errno = 0;
  energy_start_uj = em.fread(&em);
  if (!energy_start_uj && errno) {
    perror("energymon:fread");
    em.ffinish(&em);
    exit(1);
  }
  time_start_ns = energymon_gettime_ns();
  
  // execute
  printf("Executing: %s\n", cmd);
  cmd_ret = system(cmd);
  if (cmd_ret) {
    fprintf(stderr, "Warning: command exited with return code %d:\n%s\n",
            cmd_ret, cmd);
  }
  
  // get end time/energy
  errno = 0;
  energy_end_uj = em.fread(&em);
  if (!energy_end_uj && errno) {
    perror("energymon:fread");
    em.ffinish(&em);
    exit(1);
  }
  time_end_ns = energymon_gettime_ns();

  time_total_ns = time_end_ns - time_start_ns;
  energy_total_uj = energy_end_uj - energy_start_uj;
  watts = energy_total_uj * 1000.0 / time_total_ns;
  printf("Time (ns): %"PRIu64"\n", time_total_ns);
  printf("Energy (uJ): %"PRIu64"\n", energy_total_uj);
  printf("Power (W): %f\n", watts);

  // cleanup
  if (em.ffinish(&em)) {
    perror("energymon:ffinish");
  }

  // return the exit code of the command
  return cmd_ret;
}
