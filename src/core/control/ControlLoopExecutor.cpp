#include "ControlLoopExecutor.h"
#include "../FlightManager.h"
#include "../../config.h"

static float mapFloat(float x, float inMin, float inMax, float outMin, float outMax) {
    return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

void ControlLoopExecutor::init(IHALPWM* pwmOutput) {
    _pwm = pwmOutput;
    _pwmReady = false;
    if (_pwm) {
        _pwm->init();
        _pwmReady = true;
    }

    _rollAnglePID = PID(ANGLE_P_GAIN, ANGLE_I_GAIN, ANGLE_D_GAIN,
                        -MAX_YAW_RATE, MAX_YAW_RATE, PID_INTEGRAL_LIMIT);
    _pitchAnglePID = PID(ANGLE_P_GAIN, ANGLE_I_GAIN, ANGLE_D_GAIN,
                         -MAX_YAW_RATE, MAX_YAW_RATE, PID_INTEGRAL_LIMIT);
    _rollRatePID = PID(RATE_P_GAIN, RATE_I_GAIN, RATE_D_GAIN,
                       -PID_SERVO_CORRECTION_LIMIT, PID_SERVO_CORRECTION_LIMIT, PID_INTEGRAL_LIMIT);
    _pitchRatePID = PID(RATE_P_GAIN, RATE_I_GAIN, RATE_D_GAIN,
                        -PID_SERVO_CORRECTION_LIMIT, PID_SERVO_CORRECTION_LIMIT, PID_INTEGRAL_LIMIT);
    _yawRatePID = PID(RATE_P_GAIN, RATE_I_GAIN, RATE_D_GAIN,
                      -PID_SERVO_CORRECTION_LIMIT, PID_SERVO_CORRECTION_LIMIT, PID_INTEGRAL_LIMIT);

    MixerSettings settings;
    settings.rollGain = 1.0f;
    settings.pitchGain = 1.0f;
    settings.yawGain = 0.8f;
    settings.aileronTrim = 0;
    settings.elevatorTrim = 0;
    settings.rudderTrim = 0;
    settings.throttleTrim = 0;
    settings.servoMin = PWM_MIN;
    settings.servoMax = PWM_MAX;
    settings.reverseAileron = false;
    settings.reverseElevator = false;
    settings.reverseRudder = false;
    _mixer.setSettings(settings);
    resetControllers();
}

void ControlLoopExecutor::resetControllers() {
    _rollAnglePID.reset();
    _pitchAnglePID.reset();
    _rollRatePID.reset();
    _pitchRatePID.reset();
    _yawRatePID.reset();
}

void ControlLoopExecutor::applyPidGains(float angleP, float angleI, float angleD,
                                        float rateP, float rateI, float rateD) {
    _rollAnglePID.setGains(angleP, angleI, angleD);
    _pitchAnglePID.setGains(angleP, angleI, angleD);
    _rollRatePID.setGains(rateP, rateI, rateD);
    _pitchRatePID.setGains(rateP, rateI, rateD);
    _yawRatePID.setGains(rateP, rateI, rateD);
}

void ControlLoopExecutor::applyMixerSettings(const MixerSettings& settings) {
    _mixer.setSettings(settings);
}

void ControlLoopExecutor::writeSafeOutputs() {
    MixerOutput output;
    output.throttle = PWM_MIN;
    output.aileron = PWM_NEUTRAL;
    output.elevator = PWM_NEUTRAL;
    output.rudder = PWM_NEUTRAL;
    writeOutputs(output);
}

ControlCorrections ControlLoopExecutor::computeCorrections(FlightManager& flightManager, float dt) {
    float targetRoll = mapFloat(flightManager.getAileron(), 1000.0f, 2000.0f, -MAX_ROLL_ANGLE, MAX_ROLL_ANGLE);
    float targetPitch = mapFloat(flightManager.getElevator(), 1000.0f, 2000.0f, -MAX_PITCH_ANGLE, MAX_PITCH_ANGLE);
    float targetYawRate = mapFloat(flightManager.getRudder(), 1000.0f, 2000.0f, -MAX_YAW_RATE, MAX_YAW_RATE);

    float desiredRollRate = _rollAnglePID.compute(targetRoll, flightManager.getRoll(), dt);
    float desiredPitchRate = _pitchAnglePID.compute(targetPitch, flightManager.getPitch(), dt);

    float rollCorr = _rollRatePID.compute(desiredRollRate, flightManager.getGyroX(), dt);
    float pitchCorr = _pitchRatePID.compute(desiredPitchRate, flightManager.getGyroY(), dt);
    float yawCorr = _yawRatePID.compute(targetYawRate, flightManager.getGyroZ(), dt);

    ControlCorrections corrections;
    corrections.roll = rollCorr;
    corrections.pitch = pitchCorr;
    corrections.yaw = yawCorr;
    return corrections;
}

void ControlLoopExecutor::mixAndWrite(FlightManager& flightManager, const ControlCorrections& corrections) {
    MixerOutput output = _mixer.computeOutputs(
        flightManager.getThrottle(),
        corrections.roll,
        corrections.pitch,
        corrections.yaw,
        flightManager.getAileron(),
        flightManager.getElevator(),
        flightManager.getRudder()
    );
    writeOutputs(output);
}

void ControlLoopExecutor::writeOutputs(const MixerOutput& output) {
    if (!_pwm) {
        return;
    }

    HALPwmOutputs halOutput;
    halOutput.throttle = output.throttle;
    halOutput.aileron = output.aileron;
    halOutput.elevator = output.elevator;
    halOutput.rudder = output.rudder;
    _pwm->write(halOutput);
}
