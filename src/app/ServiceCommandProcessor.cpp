#include "ServiceCommandProcessor.h"

#include <common/mavlink.h>
#include "../telemetry/AeroPicoMavlinkCommands.h"

void ServiceCommandProcessor::init(const ServiceCommandProcessorContext& context) {
    _context = context;
}

void ServiceCommandProcessor::complete(uint16_t action, uint8_t result, const char* reason) {
    if (_context.mailbox) {
        _context.mailbox->complete(action, result, reason);
    }
}

void ServiceCommandProcessor::pollAsyncCompletions() {
    if (!_context.sensors || !_context.calibrationStorage) {
        return;
    }
    ImuCalibration imu = {};
    bool success = false;
    if (!_context.sensors->takeImuCalibrationResult(imu, success)) {
        return;
    }
    if (!success) {
        complete(AEROPICO_CMD_CAL_IMU, MAV_RESULT_FAILED, "IMU calibration failed");
        return;
    }
    CalibrationBlob blob = CalibrationStorage::makeBlob(imu, _context.sensors->getMagCalibration());
    if (!_context.calibrationStorage->save(blob)) {
        complete(AEROPICO_CMD_CAL_IMU, MAV_RESULT_FAILED, "IMU calibration save failed");
        return;
    }
    complete(AEROPICO_CMD_CAL_IMU, MAV_RESULT_ACCEPTED, "IMU calibration saved");
}

bool ServiceCommandProcessor::rejectIfArmed(uint16_t action, const char* reason) {
    if (_context.isArmed && _context.isArmed()) {
        complete(action, MAV_RESULT_TEMPORARILY_REJECTED, reason);
        return true;
    }
    return false;
}

void ServiceCommandProcessor::processRequest(const ServiceCommandRequest& request) {
    if (!_context.sensors) {
        complete(request.action, MAV_RESULT_FAILED, "Sensor service unavailable");
        return;
    }

    switch (request.action) {
        case AEROPICO_CMD_CAL_IMU:
            if (rejectIfArmed(request.action, "IMU calibration rejected while armed")) {
                break;
            }
            if (!_context.sensors->beginImuCalibration()) {
                complete(request.action, MAV_RESULT_TEMPORARILY_REJECTED,
                         _context.sensors->isImuCalibrationActive()
                             ? "IMU calibration already running"
                             : "IMU calibration cannot start");
            } else {
                complete(request.action, MAV_RESULT_IN_PROGRESS, "IMU calibration started");
            }
            break;

        case AEROPICO_CMD_CAL_MAG:
            if (rejectIfArmed(request.action, "Mag calibration rejected while armed")) {
                break;
            }
            if (!_context.sensors->hasMag()) {
                complete(request.action, MAV_RESULT_DENIED, "Mag calibration failed: mag missing");
                break;
            }
            if (!_context.magCalibrationActive || !*_context.magCalibrationActive) {
                _context.sensors->beginMagCalibration();
                if (_context.magCalibrationActive) {
                    *_context.magCalibrationActive = true;
                }
                complete(request.action, MAV_RESULT_IN_PROGRESS,
                         "MAG_CAL_STARTED rotate aircraft, press again to save");
                break;
            }
            {
                MagCalibration mag = _context.sensors->finishMagCalibration();
                *_context.magCalibrationActive = false;
                if (!mag.valid) {
                    complete(request.action, MAV_RESULT_FAILED,
                             "MAG_CAL_FAILED insufficient samples");
                    break;
                }
                CalibrationBlob blob = CalibrationStorage::makeBlob(_context.sensors->getImuCalibration(), mag);
                const bool saved = _context.calibrationStorage && _context.calibrationStorage->save(blob);
                complete(request.action,
                         saved ? MAV_RESULT_ACCEPTED : MAV_RESULT_FAILED,
                         saved ? "MAG_CAL_SAVED" : "MAG_CAL_SAVE_FAILED");
            }
            break;

        case AEROPICO_CMD_SERVO_TEST: {
            if (rejectIfArmed(request.action, "Servo test rejected while armed")) {
                break;
            }
            const uint8_t surface = (uint8_t)request.p2;
            const uint16_t pulse = (uint16_t)request.p3;
            const uint16_t duration = (uint16_t)request.p4;
            if (!_context.requestServoTest || !_context.requestServoTest(surface, pulse, duration)) {
                complete(request.action, MAV_RESULT_TEMPORARILY_REJECTED,
                         "Servo test failed: outputs not ready");
            } else {
                complete(request.action, MAV_RESULT_ACCEPTED, "SERVO_TEST_STARTED");
            }
            break;
        }

        default:
            complete(request.action, MAV_RESULT_UNSUPPORTED, "Unsupported queued service command");
            break;
    }
}

void ServiceCommandProcessor::process() {
    pollAsyncCompletions();
    if (!_context.mailbox) {
        return;
    }
    ServiceCommandRequest request = {};
    if (_context.mailbox->claim(request)) {
        processRequest(request);
    }
}
