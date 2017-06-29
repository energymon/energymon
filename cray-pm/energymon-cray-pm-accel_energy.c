/**
 * Read energy from Cray PM "accel_energy" file.
 *
 * @author Connor Imes
 * @date 2017-06-28
 */
#include <errno.h>
#include <inttypes.h>
#include "energymon.h"
#include "energymon-cray-pm.h"
#include "energymon-cray-pm-common.h"
#include "energymon-cray-pm-accel_energy.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT_CRAY_PM_ACCEL_ENERGY
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_cray_pm_accel_energy(em);
}
#endif

int energymon_init_cray_pm_accel_energy(energymon* em) {
  return energymon_cray_pm_common_init(em, ENERGYMON_CRAY_PM_COUNTER_ACCEL_ENERGY);
}

uint64_t energymon_read_total_cray_pm_accel_energy(const energymon* em) {
  return energymon_cray_pm_common_read_total(em);
}

int energymon_finish_cray_pm_accel_energy(energymon* em) {
  return energymon_cray_pm_common_finish(em);
}

char* energymon_get_source_cray_pm_accel_energy(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Cray Power Monitoring - accel_energy", n);
}

uint64_t energymon_get_interval_cray_pm_accel_energy(const energymon* em) {
  return energymon_cray_pm_common_get_interval(em);
}

uint64_t energymon_get_precision_cray_pm_accel_energy(const energymon* em) {
  return energymon_cray_pm_common_get_precision(em);
}

int energymon_is_exclusive_cray_pm_accel_energy(void) {
  return 0;
}

int energymon_get_cray_pm_accel_energy(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_cray_pm_accel_energy;
  em->fread = &energymon_read_total_cray_pm_accel_energy;
  em->ffinish = &energymon_finish_cray_pm_accel_energy;
  em->fsource = &energymon_get_source_cray_pm_accel_energy;
  em->finterval = &energymon_get_interval_cray_pm_accel_energy;
  em->fprecision = &energymon_get_precision_cray_pm_accel_energy;
  em->fexclusive = &energymon_is_exclusive_cray_pm_accel_energy;
  em->state = NULL;
  return 0;
}
