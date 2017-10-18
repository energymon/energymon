/**
 * Polls an energymon at the provided interval and writes the output to the
 * file specified.
 *
 * @author Connor Imes
 * @date 2015-07-09
 */
#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "energymon-default.h"
#include "energymon-time-util.h"

static volatile int running = 1;
static int count = 0;
static const char* filename = NULL;
static uint64_t interval = 0;
static int is_rewind = 1;

static const char short_options[] = "+hc:i:n";
static const struct option long_options[] = {
  {"help",      no_argument,       NULL, 'h'},
  {"count",     required_argument, NULL, 'c'},
  {"interval",  required_argument, NULL, 'i'},
  {"no-rewind", no_argument,       NULL, 'n'},
  {0, 0, 0, 0}
};

static void print_usage(int exit_code) {
  fprintf(exit_code ? stderr : stdout,
          "Usage: energymon-file-provider [OPTION]... [FILE]\n"
          "Options:\n"
          "  -h, --help               Print this message and exit\n"
          "  -c, --count=N            Stop after N reads\n"
          "  -i, --interval=US        The update interval in microseconds\n"
          "  -n, --no-rewind          Write each reading on a new line\n");
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
      case 'i':
        interval = strtoull(optarg, NULL, 0);
        if (interval == 0) {
          fprintf(stderr, "Interval must be > 0\n");
          print_usage(1);
        }
        break;
      case 'n':
        is_rewind = 0;
        break;
      case '?':
      default:
        print_usage(1);
        break;
    }
  }
  // get the file name (if set)
  if (optind < argc) {
    filename = argv[optind];
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
  FILE* fout = stdout;
  int ret = 0;

  // register the signal handler
  signal(SIGINT, shandle);

  parse_args(argc, argv);

  // initialize
  if (energymon_get_default(&em)) {
    perror("energymon_get_default");
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
  } else if (interval < min_interval) {
    fprintf(stderr,
            "Requested interval is too short, minimum available: %"PRIu64"\n",
            min_interval);
    em.ffinish(&em);
    return 1;
  }

  // get the file (if set)
  if (filename != NULL) {
    fout = fopen(filename, "w");
    if (fout == NULL) {
      perror(filename);
      return 1;
    }
  }

  // update file at regular intervals
  while (running) {
    if (count) {
      running--;
    }
    // While the interface specifies that energy values are monotonically
    // increasing, in practice that may not always hold.
    // We need to guarantee that we don't leave trailing characters if later
    // readings are a smaller order of magnitude than earlier ones.
    // Reopening the file seems to be the best way to do this.
    if (is_rewind) {
      fout = freopen(NULL, "w", fout);
      if (fout == NULL) {
        perror("Reopening output file");
        ret = 1;
        break;
      }
    }
    errno = 0;
    energy = em.fread(&em);
    if (!energy && errno) {
      perror("energymon:fread");
      // continue anyway and hope the error is transitive
      // ret = 1;
      // break;
    }
    if (fprintf(fout, "%"PRIu64"\n", energy) < 0) {
      perror("Writing to output file");
      ret = 1;
      break;
    }
    fflush(fout);
    if (running) {
      energymon_sleep_us(interval, &running);
    }
  }

  // cleanup
  if (em.ffinish(&em)) {
    perror("energymon:ffinish");
  }
  if (fout != NULL && fclose(fout)) {
    perror("Closing output file");
  }

  return ret;
}
