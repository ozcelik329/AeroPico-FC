#include "MavlinkServiceCommands.h"

#include <cmath>
#include <common/mavlink.h>
#include "../telemetry/AeroPicoMavlinkCommands.h"

void MavlinkServiceCommands::init(const MavlinkServiceContext& context) {
    _context = context;
}

bool MavlinkServiceCommands::safeForService() const {
    return _context.isArmed == nullptr || !_context.isArmed();
}

void MavlinkServiceCommands::copyReason(char* reason, size_t reasonLen, const char* text) const {
    if (!reason || reasonLen == 0) {
        return;
    }
    reason[0] = '\0';
    if (text) {
        strncpy(reason, text, reasonLen - 1);
        reason[reasonLen - 1] = '\0';
    }
}

static size_t appendScanList(char* reason,
                             size_t reasonLen,
                             size_t used,
                             const char* label,
                             uint8_t count,
                             uint8_t (*addressAt)(void*, uint8_t),
                             void* context) {
    if (!reason || reasonLen == 0 || used >= reasonLen - 1) {
        return used;
    }
    if (count == 0) {
        return used + snprintf(reason + used, reasonLen - used, " %s_NONE", label);
    }
    used += snprintf(reason + used, reasonLen - used, " %s", label);
    for (uint8_t i = 0; i < count && used < reasonLen - 1; ++i) {
        used += snprintf(reason + used, reasonLen - used, "_%02X", addressAt(context, i));
    }
    return used;
}

static uint8_t ackAddressAt(void* context, uint8_t index) {
    return static_cast<SensorManager*>(context)->getI2cAckScanAddress(index);
}

static uint8_t regAddressAt(void* context, uint8_t index) {
    return static_cast<SensorManager*>(context)->getI2cRegisterScanAddress(index);
}

void MavlinkServiceCommands::appendI2cScan(char* reason, size_t reasonLen) const {
    if (!reason || reasonLen == 0 || !_context.sensors) {
        return;
    }
    size_t used = strnlen(reason, reasonLen);
    if (used >= reasonLen - 1) {
        return;
    }
    used = appendScanList(reason,
                          reasonLen,
                          used,
                          "ACK",
                          _context.sensors->getI2cAckScanCount(),
                          ackAddressAt,
                          _context.sensors);
    appendScanList(reason,
                   reasonLen,
                   used,
                   "REG",
                   _context.sensors->getI2cRegisterScanCount(),
                   regAddressAt,
                   _context.sensors);
}

uint8_t MavlinkServiceCommands::enqueue(uint16_t action,
                                        float p2,
                                        float p3,
                                        float p4,
                                        char* reason,
                                        size_t reasonLen) {
    if (!_context.mailbox) {
        copyReason(reason, reasonLen, "Service mailbox unavailable");
        return MAV_RESULT_FAILED;
    }
    if (!_context.mailbox->submit({action, p2, p3, p4})) {
        copyReason(reason, reasonLen, "Service command busy");
        return MAV_RESULT_TEMPORARILY_REJECTED;
    }
    copyReason(reason, reasonLen, "Service command queued");
    return MAV_RESULT_ACCEPTED;
}

uint8_t MavlinkServiceCommands::handle(uint16_t action,
                                       float p2,
                                       float p3,
                                       float p4,
                                       char* reason,
                                       size_t reasonLen) {
    if (!_context.sensors) {
        copyReason(reason, reasonLen, "Service unavailable");
        return MAV_RESULT_FAILED;
    }

    switch (action) {
        case AEROPICO_CMD_CAL_IMU: {
            if (!safeForService()) {
                copyReason(reason, reasonLen, "IMU calibration rejected while armed");
                return MAV_RESULT_TEMPORARILY_REJECTED;
            }
            if (!_context.sensors->isImuAvailable()) {
                copyReason(reason, reasonLen, "IMU calibration failed: IMU missing");
                return MAV_RESULT_DENIED;
            }
            if (_context.mailbox) {
                return enqueue(action, p2, p3, p4, reason, reasonLen);
            }
            if (!_context.sensors->runBootCalibration()) {
                copyReason(reason, reasonLen, "IMU calibration failed");
                return MAV_RESULT_FAILED;
            }
            CalibrationBlob blob = CalibrationStorage::makeBlob(
                _context.sensors->getImuCalibration(),
                _context.sensors->getMagCalibration()
            );
            if (_context.calibrationStorage && !_context.calibrationStorage->save(blob)) {
                copyReason(reason, reasonLen, "IMU calibration save failed");
                return MAV_RESULT_FAILED;
            }
            copyReason(reason, reasonLen, "IMU calibration saved");
            return MAV_RESULT_ACCEPTED;
        }

        case AEROPICO_CMD_CAL_MAG: {
            if (!safeForService()) {
                copyReason(reason, reasonLen, "Mag calibration rejected while armed");
                return MAV_RESULT_TEMPORARILY_REJECTED;
            }
            if (!_context.sensors->hasMag()) {
                copyReason(reason, reasonLen, "Mag calibration failed: mag missing");
                return MAV_RESULT_DENIED;
            }
            if (_context.mailbox) {
                return enqueue(action, p2, p3, p4, reason, reasonLen);
            }
            bool& active = *_context.magCalibrationActive;
            if (!active) {
                _context.sensors->beginMagCalibration();
                active = true;
                copyReason(reason, reasonLen, "MAG_CAL_STARTED rotate aircraft, press again to save");
                return MAV_RESULT_ACCEPTED;
            }
            MagCalibration mag = _context.sensors->finishMagCalibration();
            active = false;
            if (!mag.valid) {
                copyReason(reason, reasonLen, "MAG_CAL_FAILED insufficient samples");
                return MAV_RESULT_FAILED;
            }
            CalibrationBlob blob = CalibrationStorage::makeBlob(_context.sensors->getImuCalibration(), mag);
            if (_context.calibrationStorage && !_context.calibrationStorage->save(blob)) {
                copyReason(reason, reasonLen, "MAG_CAL_SAVE_FAILED");
                return MAV_RESULT_FAILED;
            }
            copyReason(reason, reasonLen, "MAG_CAL_SAVED");
            return MAV_RESULT_ACCEPTED;
        }

        case AEROPICO_CMD_CAL_RC: {
            if (!_context.receiver) {
                copyReason(reason, reasonLen, "RC_MAP_FAIL receiver unavailable");
                return MAV_RESULT_FAILED;
            }
            if (!_context.receiver->isValid() || _context.receiver->isFailsafe()) {
                copyReason(reason, reasonLen, "RC_MAP_FAIL invalid or failsafe");
                return MAV_RESULT_DENIED;
            }

            uint8_t roll = RC_ROLL_CHANNEL;
            uint8_t pitch = RC_PITCH_CHANNEL;
            uint8_t throttle = RC_THROTTLE_CHANNEL;
            uint8_t yaw = RC_YAW_CHANNEL;
            uint8_t mode = RC_MODE_CHANNEL;
            if (_context.provideRcMapping) {
                _context.provideRcMapping(roll, pitch, throttle, yaw, mode);
            }

            const uint16_t rollUs = _context.receiver->getChannel(roll);
            const uint16_t pitchUs = _context.receiver->getChannel(pitch);
            const uint16_t throttleUs = _context.receiver->getChannel(throttle);
            const uint16_t yawUs = _context.receiver->getChannel(yaw);
            const uint16_t modeUs = _context.receiver->getChannel(mode);
            snprintf(reason,
                     reasonLen,
                     "RC_MAP_OK R=%u P=%u T=%u Y=%u M=%u",
                     rollUs, pitchUs, throttleUs, yawUs, modeUs);
            return MAV_RESULT_ACCEPTED;
        }

        case AEROPICO_CMD_SERVO_TEST: {
            if (!safeForService()) {
                copyReason(reason, reasonLen, "Servo test rejected while armed");
                return MAV_RESULT_TEMPORARILY_REJECTED;
            }
            const uint8_t surface = std::isfinite(p2) ? (uint8_t)p2 : AEROPICO_SERVO_TEST_SURFACES;
            const uint16_t pulse = std::isfinite(p3) && p3 > 0.0f ? (uint16_t)p3 : 1600;
            const uint16_t duration = std::isfinite(p4) && p4 > 0.0f ? (uint16_t)p4 : 700;
            if (_context.mailbox) {
                return enqueue(action, (float)surface, (float)pulse, (float)duration, reason, reasonLen);
            }
            if (!_context.requestServoTest || !_context.requestServoTest(surface, pulse, duration)) {
                copyReason(reason, reasonLen, "Servo test failed: outputs not ready");
                return MAV_RESULT_TEMPORARILY_REJECTED;
            }
            copyReason(reason, reasonLen, "SERVO_TEST_STARTED");
            return MAV_RESULT_ACCEPTED;
        }

        case AEROPICO_CMD_RC_MONITOR:
            copyReason(reason, reasonLen,
                       _context.receiver && _context.receiver->isValid() && !_context.receiver->isFailsafe()
                           ? "RC_MONITOR_OK"
                           : "RC_MONITOR_FAIL");
            return MAV_RESULT_ACCEPTED;

        case AEROPICO_CMD_SENSOR_CHECK: {
            _context.sensors->scanI2cBus();
            _context.sensors->refreshForDiagnostics();
            SensorCapabilityStatus caps = _context.sensors->capabilities();
            if (!caps.imuAvailable) {
                char detail[50] = {};
                snprintf(detail, sizeof(detail), "SENSOR_FAIL IMU");
                copyReason(reason, reasonLen, detail);
                return MAV_RESULT_DENIED;
            }

            copyReason(reason, reasonLen, "SENSOR_CHECK_SENT");

            if (!caps.magAvailable || !caps.baroAvailable) {
                return MAV_RESULT_ACCEPTED;
            }
            return MAV_RESULT_ACCEPTED;
        }

        case AEROPICO_CMD_PREFLIGHT_CHECK: {
            if (!_context.evaluatePreflight) {
                copyReason(reason, reasonLen, "Preflight unavailable");
                return MAV_RESULT_FAILED;
            }
            PreflightResult result = _context.evaluatePreflight();
            if (_context.lastPreflightResult) {
                *_context.lastPreflightResult = result;
            }
            copyReason(reason, reasonLen, result.canArm ? "PREFLIGHT_OK" : result.firstFailureReason);
            return result.canArm ? MAV_RESULT_ACCEPTED : MAV_RESULT_TEMPORARILY_REJECTED;
        }

        default:
            copyReason(reason, reasonLen, "Unknown AeroPico service command");
            return MAV_RESULT_UNSUPPORTED;
    }
}
