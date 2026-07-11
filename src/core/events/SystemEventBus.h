#ifndef SYSTEM_EVENT_BUS_H
#define SYSTEM_EVENT_BUS_H

#include <stddef.h>
#include <stdint.h>

enum class SystemEventType : uint8_t {
    None = 0,
    ArmStateChanged,
    ArmDenied,
    FailsafeEntered,
    FailsafeCleared,
    SensorFault,
    RcLost,
    RcRecovered,
    TimingOverrun,
    BatteryWarning,
    StorageFailure,
    BlackboxDrop
};

struct SystemEvent {
    SystemEventType type = SystemEventType::None;
    uint32_t timestampUs = 0;
    uint32_t detail = 0;
};

class SystemEventBus {
  public:
    static constexpr uint8_t CAPACITY = 16;

    bool publish(const SystemEvent& event);
    bool consume(SystemEvent& event);
    uint32_t droppedCount() const;
    size_t pendingCount() const;
    void reset();

  private:
    SystemEvent _events[CAPACITY] = {};
    alignas(4) uint32_t _head = 0;
    alignas(4) uint32_t _tail = 0;
    alignas(4) uint32_t _dropped = 0;
};

extern SystemEventBus systemEventBus;

#endif
