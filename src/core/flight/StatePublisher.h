#ifndef STATE_PUBLISHER_H
#define STATE_PUBLISHER_H

#include "../../types.h"
#include "../safety/FailsafeManager.h"

class StatePublisher {
  public:
    FlightData buildFlightData(const VehicleState& vehicle,
                               const RcInputState& rc,
                               const FailsafeDecision& failsafe) const;
    SensorState buildSensorState(const VehicleState& vehicle) const;
    ActuatorState buildActuatorState(const RcInputState& rc,
                                     bool outputsReady,
                                     bool failsafeActive) const;
};

#endif
