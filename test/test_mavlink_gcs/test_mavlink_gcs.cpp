#include <unity.h>

#ifdef MAVLINK_PARAMS_ENABLED
#undef MAVLINK_PARAMS_ENABLED
#endif

#include "telemetry/MavlinkHandler.h"

#include "../../src/telemetry/MavlinkTransport.cpp"
#include "../../src/telemetry/MavlinkHandler.cpp"

PioUart espUart;

static bool armedState;
static bool armAccepted;
static bool armCommandCalled;
static bool armCommandValue;
static bool armCommandForce;
static bool serviceCommandCalled;
static uint16_t serviceAction;
static SensorCapabilityStatus testCapabilities;

static bool provideArmState() {
    return armedState;
}

static bool handleArmCommand(bool arm, bool force, char* reason, size_t reasonLen) {
    armCommandCalled = true;
    armCommandValue = arm;
    armCommandForce = force;
    if (!armAccepted && reasonLen > 0) {
        strncpy(reason, "test denied", reasonLen - 1);
        reason[reasonLen - 1] = '\0';
    }
    armedState = armAccepted && arm;
    return armAccepted;
}

static uint8_t handleServiceCommand(uint16_t action, float p2, float p3, float p4,
                                    char* reason, size_t reasonLen) {
    (void)p2;
    (void)p3;
    (void)p4;
    serviceCommandCalled = true;
    serviceAction = action;
    if (reasonLen > 0) {
        strncpy(reason, "service ok", reasonLen - 1);
        reason[reasonLen - 1] = '\0';
    }
    return MAV_RESULT_ACCEPTED;
}

static SensorCapabilityStatus provideCapabilities() {
    return testCapabilities;
}

static bool decodeFirstMessage(mavlink_message_t& out) {
    mavlink_status_t status = {};
    for (size_t i = 0; i < mavlinkTransport.captureSize(); i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, mavlinkTransport.capture()[i], &out, &status)) {
            return true;
        }
    }
    return false;
}

static bool decodeMessageById(uint32_t msgid, mavlink_message_t& out) {
    mavlink_status_t status = {};
    for (size_t i = 0; i < mavlinkTransport.captureSize(); i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, mavlinkTransport.capture()[i], &out, &status) &&
            out.msgid == msgid) {
            return true;
        }
    }
    return false;
}

void setUp() {
    mavlinkTransport.resetCapture();
    armedState = false;
    armAccepted = true;
    armCommandCalled = false;
    armCommandValue = false;
    armCommandForce = false;
    serviceCommandCalled = false;
    serviceAction = 0;
    testCapabilities = {};
}

void test_heartbeat_reports_safety_armed_bit() {
    MavlinkHandler handler;
    armedState = true;
    handler.setArmStateProvider(provideArmState);

    handler.sendHeartbeat();

    mavlink_message_t message;
    TEST_ASSERT_TRUE(decodeFirstMessage(message));
    TEST_ASSERT_EQUAL_UINT32(MAVLINK_MSG_ID_HEARTBEAT, message.msgid);
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&message, &heartbeat);
    TEST_ASSERT_TRUE((heartbeat.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0);
}

void test_command_long_arm_sends_ack_and_invokes_handler() {
    MavlinkHandler handler;
    handler.setArmCommandHandler(handleArmCommand);
    mavlink_message_t command;
    mavlink_msg_command_long_pack(
        255, 1, &command,
        MAV_SYSTEM_ID, MAV_COMPONENT_ID,
        MAV_CMD_COMPONENT_ARM_DISARM,
        0,
        1.0f, 0, 0, 0, 0, 0, 0
    );

    handler.handleMessageForTest(command);

    TEST_ASSERT_TRUE(armCommandCalled);
    TEST_ASSERT_TRUE(armCommandValue);
    TEST_ASSERT_FALSE(armCommandForce);
    mavlink_message_t ackMessage;
    TEST_ASSERT_TRUE(decodeMessageById(MAVLINK_MSG_ID_COMMAND_ACK, ackMessage));
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&ackMessage, &ack);
    TEST_ASSERT_EQUAL_UINT16(MAV_CMD_COMPONENT_ARM_DISARM, ack.command);
    TEST_ASSERT_EQUAL_UINT8(MAV_RESULT_ACCEPTED, ack.result);
}

void test_command_long_disarm_force_flag() {
    MavlinkHandler handler;
    handler.setArmCommandHandler(handleArmCommand);
    mavlink_message_t command;
    mavlink_msg_command_long_pack(
        255, 1, &command,
        MAV_SYSTEM_ID, MAV_COMPONENT_ID,
        MAV_CMD_COMPONENT_ARM_DISARM,
        0,
        0.0f, 21196.0f, 0, 0, 0, 0, 0
    );

    handler.handleMessageForTest(command);

    TEST_ASSERT_TRUE(armCommandCalled);
    TEST_ASSERT_FALSE(armCommandValue);
    TEST_ASSERT_TRUE(armCommandForce);
}

void test_unsupported_command_sends_unsupported_ack() {
    MavlinkHandler handler;
    mavlink_message_t command;
    mavlink_msg_command_long_pack(
        255, 1, &command,
        MAV_SYSTEM_ID, MAV_COMPONENT_ID,
        MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN,
        0,
        0, 0, 0, 0, 0, 0, 0
    );

    handler.handleMessageForTest(command);

    mavlink_message_t ackMessage;
    TEST_ASSERT_TRUE(decodeMessageById(MAVLINK_MSG_ID_COMMAND_ACK, ackMessage));
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&ackMessage, &ack);
    TEST_ASSERT_EQUAL_UINT8(MAV_RESULT_UNSUPPORTED, ack.result);
}

void test_aeropico_service_command_sends_ack_and_statustext() {
    MavlinkHandler handler;
    handler.setServiceCommandHandler(handleServiceCommand);
    mavlink_message_t command;
    mavlink_msg_command_long_pack(
        255, 1, &command,
        MAV_SYSTEM_ID, MAV_COMPONENT_ID,
        MAV_CMD_USER_1,
        0,
        AEROPICO_CMD_SENSOR_CHECK, 0, 0, 0, 0, 0, 0
    );

    handler.handleMessageForTest(command);

    TEST_ASSERT_TRUE(serviceCommandCalled);
    TEST_ASSERT_EQUAL_UINT16(AEROPICO_CMD_SENSOR_CHECK, serviceAction);
    mavlink_message_t ackMessage;
    TEST_ASSERT_TRUE(decodeMessageById(MAVLINK_MSG_ID_COMMAND_ACK, ackMessage));
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&ackMessage, &ack);
    TEST_ASSERT_EQUAL_UINT16(MAV_CMD_USER_1, ack.command);
    TEST_ASSERT_EQUAL_UINT8(MAV_RESULT_ACCEPTED, ack.result);
    mavlink_message_t statusText;
    TEST_ASSERT_TRUE(decodeMessageById(MAVLINK_MSG_ID_STATUSTEXT, statusText));
}

void test_vfr_hud_reports_altitude_climb_heading_and_throttle() {
    MavlinkHandler handler;
    FlightData data = {};
    data.yaw = 370.0f;
    data.throttle = 1500;
    data.altitudeM = 123.4f;
    data.verticalSpeedMps = -1.25f;

    handler.sendVfrHud(data);

    mavlink_message_t message;
    TEST_ASSERT_TRUE(decodeFirstMessage(message));
    TEST_ASSERT_EQUAL_UINT32(MAVLINK_MSG_ID_VFR_HUD, message.msgid);
    mavlink_vfr_hud_t hud;
    mavlink_msg_vfr_hud_decode(&message, &hud);
    TEST_ASSERT_EQUAL_INT16(10, hud.heading);
    TEST_ASSERT_EQUAL_INT16(50, hud.throttle);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 123.4f, hud.alt);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.25f, hud.climb);
}

void test_gps_raw_int_reports_no_gps() {
    MavlinkHandler handler;

    handler.sendGpsRawIntNoGps();

    mavlink_message_t message;
    TEST_ASSERT_TRUE(decodeFirstMessage(message));
    TEST_ASSERT_EQUAL_UINT32(MAVLINK_MSG_ID_GPS_RAW_INT, message.msgid);
    mavlink_gps_raw_int_t gps;
    mavlink_msg_gps_raw_int_decode(&message, &gps);
    TEST_ASSERT_EQUAL_UINT8(GPS_FIX_TYPE_NO_GPS, gps.fix_type);
    TEST_ASSERT_EQUAL_UINT8(0, gps.satellites_visible);
}

void test_mission_request_list_returns_zero_count() {
    MavlinkHandler handler;
    mavlink_message_t request;
    mavlink_msg_mission_request_list_pack(
        255, 1, &request,
        MAV_SYSTEM_ID, MAV_COMPONENT_ID,
        MAV_MISSION_TYPE_MISSION
    );

    handler.handleMessageForTest(request);

    mavlink_message_t message;
    TEST_ASSERT_TRUE(decodeMessageById(MAVLINK_MSG_ID_MISSION_COUNT, message));
    mavlink_mission_count_t count;
    mavlink_msg_mission_count_decode(&message, &count);
    TEST_ASSERT_EQUAL_UINT16(0, count.count);
    TEST_ASSERT_EQUAL_UINT8(255, count.target_system);
    TEST_ASSERT_EQUAL_UINT8(1, count.target_component);
}

void test_sys_status_reports_actual_sensor_capabilities() {
    MavlinkHandler handler;
    testCapabilities.imuAvailable = true;
    testCapabilities.baroAvailable = false;
    testCapabilities.magAvailable = false;
    testCapabilities.gpsAvailable = false;
    handler.setSensorCapabilityProvider(provideCapabilities);

    handler.sendSysStatus(false, false, SensorHealth::Ok);

    mavlink_message_t message;
    TEST_ASSERT_TRUE(decodeFirstMessage(message));
    TEST_ASSERT_EQUAL_UINT32(MAVLINK_MSG_ID_SYS_STATUS, message.msgid);
    mavlink_sys_status_t status;
    mavlink_msg_sys_status_decode(&message, &status);
    TEST_ASSERT_TRUE((status.onboard_control_sensors_present & MAV_SYS_STATUS_SENSOR_3D_GYRO) != 0);
    TEST_ASSERT_TRUE((status.onboard_control_sensors_present & MAV_SYS_STATUS_SENSOR_3D_ACCEL) != 0);
    TEST_ASSERT_FALSE((status.onboard_control_sensors_present & MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE) != 0);
    TEST_ASSERT_FALSE((status.onboard_control_sensors_present & MAV_SYS_STATUS_SENSOR_3D_MAG) != 0);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_heartbeat_reports_safety_armed_bit);
    RUN_TEST(test_command_long_arm_sends_ack_and_invokes_handler);
    RUN_TEST(test_command_long_disarm_force_flag);
    RUN_TEST(test_unsupported_command_sends_unsupported_ack);
    RUN_TEST(test_aeropico_service_command_sends_ack_and_statustext);
    RUN_TEST(test_vfr_hud_reports_altitude_climb_heading_and_throttle);
    RUN_TEST(test_gps_raw_int_reports_no_gps);
    RUN_TEST(test_mission_request_list_returns_zero_count);
    RUN_TEST(test_sys_status_reports_actual_sensor_capabilities);
    return UNITY_END();
}
