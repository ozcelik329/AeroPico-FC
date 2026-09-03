#ifndef FAILSAFE_MANAGER_H
#define FAILSAFE_MANAGER_H

#include "../../types.h"

/*
 * FLIGHT FAILSAFE ARCHITECTURE ROADMAP
 * =====================================
 *
 * STATUS
 * ------
 * This block documents the intended fixed-wing failsafe architecture. It is
 * deliberately documentation-only until the existing sensor, estimator, RC,
 * actuator, telemetry, watchdog, and bench paths have completed verification.
 * Do not interpret this comment as implemented behavior.
 *
 * The current implementation couples the flight state to the arm state:
 * FlightState::Failsafe is not considered armed by FlightModeController, so
 * FlightControlTask writes disarmed safe outputs. That behavior is appropriate
 * for bench safety but is not the intended airborne fixed-wing behavior. It
 * must not be changed piecemeal. The state model, output policy, telemetry, and
 * tests below must be introduced together.
 *
 * CORE SAFETY RULE
 * ----------------
 * Failsafe is not the same operation as disarm.
 *
 * - Disarm means propulsion and actuator outputs enter their configured
 *   ground-safe state.
 * - Failsafe means the aircraft is still armed, but control authority moves to
 *   a reason-specific safety action.
 * - An airborne fixed-wing aircraft must not lose all control surfaces merely
 *   because RC, barometer, magnetometer, GPS, or another recoverable capability
 *   is unavailable.
 * - Explicit pilot disarm, a configured termination policy, or an unrecoverable
 *   actuator/output condition are separate decisions.
 *
 * TARGET STATE MODEL
 * ------------------
 * Arm state, commanded flight mode, and failsafe action must be independent:
 *
 *   ArmState:       Disarmed | Armed
 *   FlightMode:     Manual | Stabilize | future navigation modes
 *   FailsafeAction: None | ManualFallback | EmergencyGlide |
 *                   CapabilityDegraded | OutputSafe | Terminate
 *
 * A typical RC-loss state must therefore be represented as:
 *
 *   ArmState       = Armed
 *   FlightMode     = Stabilize (last commanded mode)
 *   FailsafeAction = EmergencyGlide
 *   PrimaryReason  = RC_LOSS
 *
 * Do not encode ArmState by asking whether a FlightState enum happens to equal
 * ArmedManual. ARMED_FAILSAFE must remain armed from the output task's point of
 * view. A possible runtime state machine is:
 *
 *   Disarmed -> ReadyToArm -> ArmedNormal -> ArmedFailsafe
 *                                      \-> RecoveryPending -> ArmedNormal
 *
 * The exact enum names may differ, but the independent concepts must remain.
 *
 * SINGLE DECISION AUTHORITY
 * -------------------------
 * FailsafeManager must be the only producer of the authoritative decision.
 * Normal arm, Force Arm, preflight reporting, the runtime state machine,
 * actuator output selection, MAVLink telemetry, event logging, and the
 * Configurator must consume the same fresh decision snapshot.
 *
 * The final decision must contain at least:
 *
 *   bool active;
 *   uint16_t observedReasonMask;  // every currently observed fault
 *   uint16_t effectiveReasonMask; // after the explicit policy is applied
 *   FailsafeReason primaryReason;
 *   FailsafeAction action;
 *   FailsafeSeverity severity;
 *   uint32_t firstObservedUs;
 *   uint32_t lastUpdatedUs;
 *   uint32_t activeSinceUs;
 *
 * The snapshot used by an arm command must be refreshed or age-checked at the
 * command boundary. A stale preflight/failsafe snapshot must never authorize or
 * reject arm. Cross-core flags and snapshots must use the existing atomic,
 * seqlock, or blackboard ownership pattern; plain shared bool values are not an
 * acceptable cross-core contract.
 *
 * REASON-TO-ACTION POLICY
 * -----------------------
 * The policy is capability-based. Losing a sensor must disable only the modes
 * that require that sensor unless a more serious fault makes control unsafe.
 *
 * RC loss:
 * - If the estimator and actuator outputs are healthy, remain armed and enter
 *   EmergencyGlide.
 * - Invalid RC values must never be mixed into outputs while RC loss is active.
 * - When RC becomes stable again, use the recovery policy below before handing
 *   authority back to the pilot.
 *
 * Battery critical:
 * - Keep control surfaces active.
 * - Select the configured throttle limit or idle output and enter an emergency
 *   landing/glide action.
 * - Battery monitoring disabled by module setup must not create this reason.
 *
 * Barometer loss:
 * - Keep Manual and attitude Stabilize available.
 * - Disable altitude-dependent control and navigation features.
 * - Barometer loss alone must not disarm the aircraft.
 *
 * Magnetometer or GPS loss:
 * - Keep Manual and attitude Stabilize available when IMU/estimator data are
 *   healthy.
 * - Disable only heading/navigation features that require the missing source.
 * - MAG/GPS disabled by module setup must not be reported as a fault.
 *
 * Short IMU or estimator interruption:
 * - A very short, bounded hold-last-valid window may bridge one transient.
 * - PID integration must not continue from stale data without bounds.
 * - The hold duration must be explicit, tested, and shorter than the interval
 *   in which stale outputs can destabilize the airframe.
 *
 * Persistent IMU or estimator loss:
 * - If valid RC remains available and the actuator path is healthy, fall back
 *   to direct Manual pass-through because Stabilize is no longer trustworthy.
 * - If RC is also unavailable, use preconfigured bounded emergency outputs.
 * - Do not claim an attitude-controlled glide when attitude is unavailable.
 *
 * Timing fault:
 * - A recoverable budget warning may degrade expensive noncritical work and
 *   remain under runtime control.
 * - A persistent control-loop failure belongs to the watchdog path.
 * - Timing severity and persistence must be explicit; one transient overrun
 *   must not silently become permanent termination.
 *
 * Actuator/output fault:
 * - Never bypass a confirmed critical actuator/output fault with Force Arm.
 * - Use only outputs known to be available. Throttle must move to its configured
 *   safe value when propulsion control cannot be trusted.
 * - Termination, if ever supported, must be an explicit product policy and not
 *   an accidental consequence of entering Failsafe.
 *
 * EMERGENCY GLIDE AUTHORITY
 * -------------------------
 * EmergencyGlide is an internal failsafe action, not a normal mode selectable
 * from the mode switch or Configurator.
 *
 * - On RC loss, the flight controller owns roll, pitch, throttle, and rudder
 *   outputs because no valid pilot command exists.
 * - On battery critical with valid RC, pilot surface control may remain active
 *   while throttle is limited by policy.
 * - On barometer/MAG/GPS loss with valid RC, pilot control remains available and
 *   only unsupported automatic capabilities are removed.
 * - On IMU loss with valid RC, use Manual pass-through instead of pretending
 *   Stabilize or EmergencyGlide can estimate attitude.
 *
 * Initial EmergencyGlide behavior should be intentionally small and testable:
 *
 * - configured roll target, normally wings level;
 * - configured airframe-specific pitch target;
 * - configured throttle idle/minimum value;
 * - neutral rudder unless a tested policy says otherwise;
 * - attitude PID only while IMU and estimator health permit it.
 *
 * Never hard-code one pitch angle as safe for every airframe. A later airspeed-
 * aware implementation may target best-glide airspeed, but the first version
 * must use conservative airframe parameters verified in simulation and flight.
 * Candidate runtime parameters include:
 *
 *   FS_RC_ACTION, FS_GLIDE_ROLL, FS_GLIDE_PITCH, FS_GLIDE_THR,
 *   FS_TRIGGER_MS, FS_RECOVER_MS, FS_SENSOR_HOLD_MS
 *
 * RECOVERY AND HYSTERESIS
 * -----------------------
 * Fault entry and recovery require independent debounce windows.
 *
 * - A source must remain invalid for its configured trigger interval before a
 *   recoverable transient becomes an active failsafe.
 * - A recovered source must remain valid for its configured recovery interval.
 * - Authority transfer from EmergencyGlide to pilot control must be bumpless:
 *   reset/freeze integrators as required and blend outputs over a bounded time.
 * - Flapping input health must not repeatedly switch modes or outputs.
 * - Critical reasons may latch until explicit disarm or operator acknowledgement.
 * - Telemetry must show ENTERED, ACTIVE, RECOVERY_PENDING, RECOVERED, and the
 *   reason/action involved.
 *
 * PREFLIGHT VERSUS RUNTIME
 * ------------------------
 * Preflight answers whether entering Armed is permitted. Runtime failsafe
 * answers what an already armed aircraft must do after a fault. They use the
 * same health inputs but do not necessarily choose the same action.
 *
 * - A required IMU missing on the ground blocks normal arm.
 * - The same IMU disappearing in flight selects a bounded runtime fallback; it
 *   must not accidentally invoke the ground disarm path.
 * - Disabled modules are removed from both preflight requirements and runtime
 *   fault generation.
 * - Normal Arm never bypasses active failsafe reasons.
 *
 * BENCH FORCE ARM POLICY
 * ----------------------
 * GP20-GP21 authorization only permits an explicit Force Arm command. It must
 * not alter Normal Arm and must not globally clear runtime failsafe decisions.
 *
 * - RC loss, sensor invalid, and estimator invalid may be bypassed only for the
 *   explicitly authorized bench session.
 * - Confirmed actuator faults and serious timing/watchdog faults are never
 *   bypassable.
 * - Removing the jumper cancels authorization immediately.
 * - Disarm ends the Force Arm session.
 * - Telemetry must expose BENCH_FORCE_ARM while the session is active.
 * - Propellers must remain removed during every bench Force Arm test.
 *
 * WATCHDOG AND IN-FLIGHT RESTART
 * ------------------------------
 * Runtime failsafe and the watchdog solve different failures and may coexist:
 *
 * - Failsafe acts while the scheduler and control software are still running.
 * - The watchdog resets the processor when required execution/heartbeat gates
 *   stop progressing.
 *
 * Do not use watchdog reset as a normal failsafe action. Do not implement blind
 * automatic re-arm after every reboot. Initial release behavior must remain a
 * conservative cold boot until ordinary failsafe has been flight-validated.
 *
 * A future InFlightRecovery design requires all of the following as one feature:
 *
 * - reset-cause validation proving a watchdog reset, not power-on, brownout,
 *   manual reboot, firmware upload, or corrupt reset state;
 * - retained state with magic, schema version, CRC, monotonic counter, armed
 *   state, last mode/action, attitude/navigation context, and freshness data;
 * - reboot-loop protection and at most one bounded recovery attempt;
 * - selective fast initialization, not a blanket bypass of every check;
 * - minimum IMU/estimator/output sanity before control output restoration;
 * - an explicit InFlightRecovery state and unambiguous telemetry/event logging;
 * - hardware evidence that PWM/output interruption during RP2350 reboot is
 *   acceptable, or an independent actuator/IO controller that can hold safe
 *   outputs while the main processor restarts.
 *
 * TELEMETRY AND CONFIGURATOR CONTRACT
 * -----------------------------------
 * The Configurator must display authoritative firmware state, never infer it
 * from old I2C scans or sticky UI evidence. At minimum publish:
 *
 * - ArmState;
 * - commanded FlightMode;
 * - active FailsafeAction and severity;
 * - observed and effective reason masks;
 * - primary reason;
 * - activation age and recovery state;
 * - BENCH_FORCE_ARM/InFlightRecovery flags when applicable.
 *
 * STATUSTEXT is useful for human-readable transitions, but a structured MAVLink
 * payload or stable encoded status field must carry the current snapshot. Rate-
 * limit repeated messages by transition/reason, not by hiding state changes.
 *
 * IMPLEMENTATION ROADMAP AND RELEASE GATES
 * ----------------------------------------
 * Phase 0 - finish current verification before changing behavior:
 * - sensor hot-plug/liveness and health reporting;
 * - estimator validity and stale-data behavior;
 * - RC enable/disable propagation and mode selection;
 * - actuator/servo output ownership and bench safety;
 * - watchdog build separation and USB/telemetry stability.
 *
 * Phase 1 - state-model refactor:
 * - separate ArmState, FlightMode, and FailsafeAction;
 * - make one FailsafeDecision snapshot authoritative;
 * - ensure ArmedFailsafe remains armed in FlightControlTask;
 * - preserve current disarmed bench outputs exactly.
 *
 * Phase 2 - deterministic fallback actions:
 * - implement capability degradation and ManualFallback;
 * - implement parameterized EmergencyGlide;
 * - add trigger/recovery hysteresis and bumpless transfer;
 * - define reason priority without discarding secondary reason bits.
 *
 * Phase 3 - product observability:
 * - publish structured reason/action state through MAVLink;
 * - update Configurator status, checklist, event log, and tests;
 * - log every transition in blackbox with timestamp and reason mask.
 *
 * Phase 4 - verification:
 * - native unit and state-transition tests for every reason combination;
 * - fault injection for RC, each sensor, estimator, battery, timing, and output;
 * - bench tests with propeller removed and measured PWM outputs;
 * - simulation/HIL validation of glide, recovery, and mode degradation;
 * - controlled flight-envelope expansion with documented acceptance criteria.
 *
 * Phase 5 - optional InFlightRecovery:
 * - begin only after Phases 1-4 are flight-proven;
 * - validate retained-state integrity, reset classification, output continuity,
 *   recovery-loop prevention, and safe fallback when recovery is rejected.
 *
 * No phase is release-ready solely because it compiles. Flight release requires
 * repeatable evidence from tests appropriate to that phase.
 */

enum FailsafeReason : uint16_t {
    FailsafeNone = 0,
    FailsafeRcLoss = 1u << 0,
    FailsafeSensorInvalid = 1u << 1,
    FailsafeEstimatorInvalid = 1u << 2,
    FailsafeTiming = 1u << 3,
    FailsafeBatteryCritical = 1u << 4,
    FailsafeActuator = 1u << 5
};

static constexpr uint16_t FAILSAFE_BENCH_BYPASS_MASK =
    FailsafeRcLoss | FailsafeSensorInvalid | FailsafeEstimatorInvalid;

struct FailsafePolicy {
    bool rcRequired = true;
    uint16_t bypassMask = FailsafeNone;
};

struct FailsafeDecision {
    bool active = false;
    const char* reason = "OK";
    uint16_t reasons = FailsafeNone;
    uint16_t observedReasons = FailsafeNone;
    uint32_t timestampUs = 0;
};

class FailsafeManager {
  public:
    void init();
    FailsafeDecision evaluate(const FlightData& data,
                              const FailsafePolicy& policy = {}) const;
    static const char* reasonToken(uint16_t reasons);
};

#endif
