#include <unity.h>

#include "core/sensors/SensorPipeline.h"

#include "../../src/estimators/BaroAltitudeEstimator.cpp"
#include "../../src/estimators/BaroVerticalKalman.cpp"
#include "../../src/estimators/ComplementaryEstimator.cpp"
#include "../../src/core/sensors/SensorFusion.cpp"
#include "../../src/core/sensors/SensorPipeline.cpp"

class FakeImuDriver : public IImuDriver {
  public:
    void init() override {
        _sample = {};
        _sample.ax = 0.0f;
        _sample.ay = 0.0f;
        _sample.az = 1.0f;
        _sample.tempC = 25.0f;
        _sample.pressureHpa = 1013.25f;
        _sample.baroValid = true;
        _sample.valid = true;
        _sample.health = SensorHealth::Ok;
        _sample.qualityScore = 100;
    }

    void update() override {
        _sample.timestamp += 10000;
    }

    SensorBuffer getLatest() override {
        return _sample;
    }

    bool isImuAvailable() const override { return true; }
    bool isDmaOk() const override { return true; }
    bool runBootCalibration() override { return true; }

    void setPressure(float pressureHpa, bool valid = true) {
        _sample.pressureHpa = pressureHpa;
        _sample.baroValid = valid;
    }

  private:
    SensorBuffer _sample = {};
};

void test_sensor_pipeline_publishes_baro_vertical_kalman_altitude() {
    FakeImuDriver driver;
    SensorPipeline pipeline;

    pipeline.init(&driver);
    VehicleState first = pipeline.update();
    TEST_ASSERT_TRUE(first.estimatorValid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, first.altitudeM);

    driver.setPressure(1012.25f);
    VehicleState second = pipeline.update();

    TEST_ASSERT_TRUE(second.estimatorValid);
    TEST_ASSERT_EQUAL((int)SensorHealth::Ok, (int)second.estimatorHealth);
    TEST_ASSERT_TRUE(second.altitudeM >= 0.0f);
}

void test_sensor_pipeline_keeps_estimator_prediction_without_baro() {
    FakeImuDriver driver;
    SensorPipeline pipeline;

    pipeline.init(&driver);
    pipeline.update();
    driver.setPressure(1012.25f);
    pipeline.update();
    driver.setPressure(0.0f, false);

    VehicleState state = pipeline.update();

    TEST_ASSERT_TRUE(state.estimatorValid);
    TEST_ASSERT_EQUAL((int)SensorHealth::Ok, (int)state.estimatorHealth);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_pipeline_publishes_baro_vertical_kalman_altitude);
    RUN_TEST(test_sensor_pipeline_keeps_estimator_prediction_without_baro);
    return UNITY_END();
}
