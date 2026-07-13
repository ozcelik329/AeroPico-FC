#include "ControlLoopExecutor.h"
#include "../flight/FlightManager.h"
#include "board/Config.h"
#include "../../utils/FastMath.h"

void ControlLoopExecutor::init(IHALPWM* pwmOutput) {
    _pwm = pwmOutput;
    _pwmReady = false;
    if (_pwm) {
        _pwm->init();
        _pwmReady = _pwm->isReady();
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

void ControlLoopExecutor::writeServoTestOutputs(uint8_t surface, uint16_t pulseUs) {
    MixerOutput output;
    output.throttle = PWM_MIN;
    output.aileron = PWM_NEUTRAL;
    output.elevator = PWM_NEUTRAL;
    output.rudder = PWM_NEUTRAL;

    const uint16_t clampedPulse = constrain(pulseUs, (uint16_t)PWM_MIN, (uint16_t)PWM_MAX);
    switch (surface) {
        case 1:
            output.aileron = clampedPulse;
            break;
        case 2:
            output.elevator = clampedPulse;
            break;
        case 3:
            output.rudder = clampedPulse;
            break;
        case 4:
            output.throttle = clampedPulse;
            break;
        default:
            output.aileron = clampedPulse;
            output.elevator = clampedPulse;
            output.rudder = clampedPulse;
            break;
    }
    writeOutputs(output);
}

ControlCorrections ControlLoopExecutor::computeCorrections(const FlightData& data, float dt) {
    const float rollStick = AeroPicoFastMath::pwmToUnit(data.aileron);
    const float pitchStick = AeroPicoFastMath::pwmToUnit(data.elevator);
    const float yawStick = AeroPicoFastMath::pwmToUnit(data.rudder);
    float targetRoll = rollStick * MAX_ROLL_ANGLE;
    float targetPitch = pitchStick * MAX_PITCH_ANGLE;
    float targetYawRate = yawStick * MAX_YAW_RATE;

    float desiredRollRate = _rollAnglePID.compute(targetRoll, data.roll, dt);
    float desiredPitchRate = _pitchAnglePID.compute(targetPitch, data.pitch, dt);

    float rollCorr = _rollRatePID.compute(desiredRollRate, data.gyroX, dt);
    float pitchCorr = _pitchRatePID.compute(desiredPitchRate, data.gyroY, dt);
    float yawCorr = _yawRatePID.compute(targetYawRate, data.gyroZ, dt);

    ControlCorrections corrections;
    corrections.roll = rollCorr;
    corrections.pitch = pitchCorr;
    corrections.yaw = yawCorr;
    return corrections;
}

void ControlLoopExecutor::mixAndWrite(const FlightData& data, const ControlCorrections& corrections) {
    MixerOutput output = _mixer.computeOutputs(
        data.throttle,
        corrections.roll,
        corrections.pitch,
        corrections.yaw,
        data.aileron,
        data.elevator,
        data.rudder
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
