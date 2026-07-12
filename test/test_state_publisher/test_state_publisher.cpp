#include <unity.h>

#include "core/flight/StatePublisher.h"

#include "../../src/core/flight/StatePublisher.cpp"

static VehicleState vehicleState() {
    VehicleState vehicle = {};
    vehicle.rollDeg = 1.0f;
    vehicle.pitchDeg = 2.0f;
    vehicle.yawDeg = 3.0f;
    vehicle.gyroX = 4.0f;
    vehicle.gyroY = 5.0f;
    vehicle.gyroZ = 6.0f;
    vehicle.altitudeM = 42.0f;
    vehicle.verticalSpeedMps = 1.5f;
    vehicle.sensorHealth = SensorHealth::Ok;
    vehicle.estimatorHealth = SensorHealth::Ok;
    vehicle.sensorQualityScore = 92;
    vehicle.sensorAgeUs = 1200;
    vehicle.timestampUs = 1234;
    vehicle.valid = true;
    vehicle.estimatorValid = true;
    return vehicle;
}

static RcInputState rcState() {
    RcInputState rc = {};
    rc.aileron = 1510;
    rc.elevator = 1520;
    rc.throttle = 1300;
    rc.rudder = 1490;
    rc.controlMode = ControlMode::Manual;
    rc.failsafe = false;
    rc.overrideActive = false;
    rc.timestampMs = 12;
    return rc;
}

void test_state_publisher_builds_flight_data() {
    StatePublisher publisher;

    FlightData data = publisher.buildFlightData(vehicleState(), rcState(), {true, "test"});

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, data.roll);
    TEST_ASSERT_EQUAL_UINT16(1300, data.throttle);
    TEST_ASSERT_TRUE(data.failsafe);
    TEST_ASSERT_EQUAL((int)SensorHealth::Ok, (int)data.sensorHealth);
    TEST_ASSERT_EQUAL_UINT8(92, data.sensorQualityScore);
    TEST_ASSERT_EQUAL_UINT32(1200, data.sensorAgeUs);
    TEST_ASSERT_TRUE(data.estimatorValid);
    TEST_ASSERT_EQUAL((int)ControlMode::Manual, (int)data.controlMode);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 42.0f, data.altitudeM);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, data.verticalSpeedMps);
}

void test_state_publisher_builds_sensor_state() {
    StatePublisher publisher;

    SensorState state = publisher.buildSensorState(vehicleState());

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_EQUAL_UINT32(1234, state.timestampUs);
    TEST_ASSERT_EQUAL_UINT8(92, state.sensorQualityScore);
    TEST_ASSERT_EQUAL_UINT32(1200, state.sensorAgeUs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, state.gyroZ);
    TEST_ASSERT_TRUE(state.estimatorValid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 42.0f, state.altitudeM);
}

void test_state_publisher_builds_actuator_state() {
    StatePublisher publisher;

    ActuatorState state = publisher.buildActuatorState(rcState(), true, false);

    TEST_ASSERT_TRUE(state.outputsReady);
    TEST_ASSERT_FALSE(state.failsafe);
    TEST_ASSERT_EQUAL_UINT16(1510, state.aileron);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_state_publisher_builds_flight_data);
    RUN_TEST(test_state_publisher_builds_sensor_state);
    RUN_TEST(test_state_publisher_builds_actuator_state);
    return UNITY_END();
}
