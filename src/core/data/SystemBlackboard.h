#ifndef SYSTEM_BLACKBOARD_H
#define SYSTEM_BLACKBOARD_H

#include "../../types.h"
#include "SeqlockTopic.h"

class SystemBlackboard {
  public:
    SeqlockTopic<SensorState> sensor;
    SeqlockTopic<VehicleState> vehicle;
    SeqlockTopic<RcInputState> rc;
    SeqlockTopic<ActuatorState> actuator;
    SeqlockTopic<NavigationState> navigation;
    SeqlockTopic<FlightData> telemetry;
};

extern SystemBlackboard systemBlackboard;

#endif
