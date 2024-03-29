/**
 * Polls an energymon at the requested interval and prints the average power
 * over that interval.
 *
 * @author Connor Imes
 * @date 2016-08-30
 */
#define _GNU_SOURCE
#include <errno.h>
#include <float.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "energymon.h"
#include "energymon-get.h"
#include "energymon-time-util.h"

#ifndef ENERGYMON_UTIL_PREFIX
#define ENERGYMON_UTIL_PREFIX "energymon"
#endif

static const int IGNORE_INTERRUPT = 0;

static volatile uint64_t running = 1;
static int count = 0;
static const char* filename = NULL;
static int force = 0;
static int summarize = 0;
static uint64_t interval = 0;

static const char short_options[] = "hc:f:Fi:s";
static const struct option long_options[] = {
  {"help",      no_argument,       NULL, 'h'},
  {"count",     required_argument, NULL, 'c'},
  {"file",      required_argument, NULL, 'f'},
  {"force",     no_argument,       NULL, 'F'},
  {"interval",  required_argument, NULL, 'i'},
  {"summarize", no_argument,       NULL, 's'},
  {0, 0, 0, 0}
};

__attribute__ ((noreturn))
static void print_usage(int exit_code) {
  fprintf(exit_code ? stderr : stdout,
          "Usage: "ENERGYMON_UTIL_PREFIX"-power-poller [OPTION]...\n\n"
          "Prints the average power in Watts at regular intervals.\n\n"
          "Power 'P' is computed as P=E/t, where 'E' is the measured energy difference and\n"
          "'t' is the actual time elapsed between updates.\n\n"
          "If no additional options are specified, average power is printed to standard\n"
          "output at the implementation's minimum update interval, and the program loops\n"
          "until interrupted with CTRL-C.\n"
          "Note that using the default interval is not always desirable, as it may be too\n"
          "fast and cause unnecessary overhead.\n"
          "Also, computing power at the minimum update interval can result in noisy data.\n\n"
          "Variation in the number of internal EnergyMon updates between reads causes\n"
          "noise in the reported power values. If no internal updates are accomplished\n"
          "between reads, the average power will be reported as 0 and the next non-zero\n"
          "value reported may be roughly X times larger than normal, where X is similar to\n"
          "the number of preceding zero-valued reports.\n\n"
          "Options:\n"
          "  -h, --help               Print this message and exit\n"
          "  -c, --count=N            Stop after N reads\n"
          "  -f, --file=FILE          The output file\n"
          "  -F, --force              Force updates faster than the EnergyMon claims\n"
          "  -i, --interval=US        The update interval in microseconds (> 0)\n"
          "  -s, --summarize          Print out a summary at completion\n");
  exit(exit_code);
}

static void parse_args(int argc, char** argv) {
  int c;
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage(0);
        break;
      case 'c':
        count = 1;
        running = strtoull(optarg, NULL, 0);
        break;
      case 'f':
        filename = optarg;
        break;
      case 'F':
        force = 1;
        break;
      case 'i':
        interval = strtoull(optarg, NULL, 0);
        if (interval == 0) {
          fprintf(stderr, "Interval must be > 0\n");
          print_usage(1);
        }
        break;
      case 's':
        summarize = 1;
        break;
      case '?':
      default:
        print_usage(1);
        break;
    }
  }
}

static void shandle(int sig) {
  switch (sig) {
    case SIGTERM:
    case SIGINT:
#ifdef SIGQUIT
    case SIGQUIT:
#endif
#ifdef SIGKILL
    case SIGKILL:
#endif
#ifdef SIGHUP
    case SIGHUP:
#endif
      running = 0;
    default:
      break;
  }
}

int main(int argc, char** argv) {
  energymon em;
  uint64_t min_interval;
  uint64_t energy;
  uint64_t energy_last;
  uint64_t last_us;
  uint64_t exec_us;
  float power;
  uint64_t n = 0;
  float pmin = FLT_MAX;
  float pmax = FLT_MIN;
  float pavg = 0;
  float pavg_last = 0;
  float pstd = 0;
  FILE* fout = stdout;
  int ret = 0;

  // register the signal handler
  signal(SIGINT, shandle);

  parse_args(argc, argv);

  // initialize the energy monitor
  if (energymon_get(&em)) {
    return 1;
  }
  if (em.finit(&em)) {
    perror("energymon:finit");
    return 1;
  }

  // get the update interval
  min_interval = em.finterval(&em);
  if (interval == 0) {
    interval = min_interval;
  } else if (interval < min_interval && !force) {
    fprintf(stderr,
            "Requested interval is too short, minimum available: %"PRIu64"\n",
            min_interval);
    fprintf(stderr, "Use -F/--force to ignore this check\n");
    em.ffinish(&em);
    return 1;
  }

  // open the output file
  if (filename != NULL) {
    fout = fopen(filename, "w");
    if (fout == NULL) {
      perror(filename);
      em.ffinish(&em);
      return 1;
    }
  }

  // output at regular intervals
  energy_last = em.fread(&em);
  last_us = energymon_gettime_us();
  energymon_sleep_us(interval, &IGNORE_INTERRUPT);
  while (running) {
    if (count) {
      running--;
    }
    energy = em.fread(&em);
    exec_us = energymon_gettime_elapsed_us(&last_us);
    power = (energy - energy_last) / ((float) exec_us);
    if (fprintf(fout, "%f\n", power) < 0) {
      ret = 1;
      if (filename == NULL) {
        // should never happen writing to stdout
        perror("Writing");
      } else {
        perror(filename);
      }
      break;
    }
    fflush(fout);
    energy_last = energy;
    if (power > pmax) {
      pmax = power;
    }
    if (power < pmin) {
      pmin = power;
    }
    // get the running average and data to compute stddev
    pavg_last = pavg;
    n++;
    if (n == 1) {
      pavg = power;
    } else {
      pavg = pavg + ((power - pavg) / n);
    }
    pstd = pstd + (power - pavg) * (power - pavg_last);
    if (running) {
      energymon_sleep_us(interval, &IGNORE_INTERRUPT);
    }
  }

  if (summarize) {
    fprintf(fout, "Samples: %"PRIu64"\n", n);
    fprintf(fout, "Pavg: %f\n", pavg);
    fprintf(fout, "Pmax: %f\n", pmax);
    fprintf(fout, "Pmin: %f\n", n > 0 ? pmin : 0);
    fprintf(fout, "Pstdev: %f\n", n <= 1 ? 0 : (sqrt(pstd / (n - 1))));
    fprintf(fout, "Joules: %f\n", n * pavg * (interval / 1000000.0f));
  }

  // cleanup
  if (filename != NULL && fclose(fout)) {
    perror(filename);
  }
  if (em.ffinish(&em)) {
    perror("em.ffinish");
  }

  return ret;
}
