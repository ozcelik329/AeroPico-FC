#ifndef CONFIGURATOR_PARAM_RUNTIME_H
#define CONFIGURATOR_PARAM_RUNTIME_H

#ifdef MAVLINK_PARAMS_ENABLED

#include "../telemetry/ParamManager.h"

namespace ConfiguratorParamRuntime {
void bindAndApply(ParamManager& params,
                  ParamManager::PidGainsApplyHandler pidHandler,
                  ParamManager::MixerSettingsApplyHandler mixerHandler,
                  ParamManager::FailsafeTimeoutApplyHandler failsafeHandler,
                  ParamManager::RcMappingApplyHandler rcMappingHandler,
                  ParamManager::MavlinkRatesApplyHandler mavlinkRatesHandler,
                  ParamManager::BlackboxRateApplyHandler blackboxRateHandler,
                  ParamManager::PreflightQualityApplyHandler preflightQualityHandler,
                  ParamManager::BatteryProfileApplyHandler batteryProfileHandler);
}

#endif

#endif
