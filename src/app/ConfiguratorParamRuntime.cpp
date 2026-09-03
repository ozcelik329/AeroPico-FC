#include "ConfiguratorParamRuntime.h"

#ifdef MAVLINK_PARAMS_ENABLED

void ConfiguratorParamRuntime::bindAndApply(ParamManager& params,
                                            ParamManager::PidGainsApplyHandler pidHandler,
                                            ParamManager::MixerSettingsApplyHandler mixerHandler,
                                            ParamManager::FailsafeTimeoutApplyHandler failsafeHandler,
                                            ParamManager::RcMappingApplyHandler rcMappingHandler,
                                            ParamManager::MavlinkRatesApplyHandler mavlinkRatesHandler,
                                            ParamManager::BlackboxRateApplyHandler blackboxRateHandler,
                                            ParamManager::PreflightQualityApplyHandler preflightQualityHandler,
                                            ParamManager::BatteryProfileApplyHandler batteryProfileHandler,
                                            ParamManager::ModuleSetupApplyHandler moduleSetupHandler) {
    params.setPidGainsApplyHandler(pidHandler);
    params.setMixerSettingsApplyHandler(mixerHandler);
    params.setFailsafeTimeoutApplyHandler(failsafeHandler);
    params.setRcMappingApplyHandler(rcMappingHandler);
    params.setMavlinkRatesApplyHandler(mavlinkRatesHandler);
    params.setBlackboxRateApplyHandler(blackboxRateHandler);
    params.setPreflightQualityApplyHandler(preflightQualityHandler);
    params.setBatteryProfileApplyHandler(batteryProfileHandler);
    params.setModuleSetupApplyHandler(moduleSetupHandler);

    pidHandler(params.getAngleP(), params.getAngleI(), params.getAngleD(),
               params.getRateP(), params.getRateI(), params.getRateD());
    mixerHandler(params.getMixerSettings());
    failsafeHandler(params.getFailsafeTimeoutMs());
    rcMappingHandler(params.getRcRollChannel(), params.getRcPitchChannel(),
                     params.getRcThrottleChannel(), params.getRcYawChannel(),
                     params.getRcModeChannel());
    mavlinkRatesHandler(params.getMavlinkAttitudeHz(), params.getMavlinkRcHz(), params.getMavlinkSysStatusHz());
    blackboxRateHandler(params.getBlackboxLogHz());
    preflightQualityHandler(params.getPreflightMinQuality());
    batteryProfileHandler(params.getBatteryCellCount(), params.getBatteryNominalVoltage(),
                          params.getBatteryCapacityMah(), params.getBatteryCRating(),
                          params.getBatteryLowVoltage(), params.getBatteryBrownoutVoltage(),
                          params.getBatteryMaxVoltage());
    moduleSetupHandler();
}

#endif
