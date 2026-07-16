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

        case AEROPICO_CMD_CAL_RC:
            copyReason(reason, reasonLen, "RC calibration uses runtime RC mapping params");
            return MAV_RESULT_UNSUPPORTED;

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
            SensorCapabilityStatus caps = _context.sensors->capabilities();
            char busSummary[96];
            _context.sensors->formatBusProbeSummary(busSummary, sizeof(busSummary));
            if (!caps.imuAvailable) {
                copyReason(reason, reasonLen, busSummary[0] ? busSummary : _context.sensors->getFaultText());
                return MAV_RESULT_DENIED;
            }
            if (!caps.magAvailable || !caps.baroAvailable) {
                copyReason(reason, reasonLen, busSummary[0] ? busSummary : "SENSOR_CHECK_PARTIAL optional sensor missing");
                return MAV_RESULT_ACCEPTED;
            }
            copyReason(reason, reasonLen, busSummary[0] ? busSummary : "SENSOR_CHECK_OK");
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
