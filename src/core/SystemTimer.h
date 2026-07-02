#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H
#include <Arduino.h>
#include "def.h"
#include "pico/multicore.h"

class SystemTimer {
public:
    static constexpr uint32_t LOOP_TIME_US = LOOP_TIME;

    enum LoopPhase {
        PHASE_CONSUME = 0,
        PHASE_PID,
        PHASE_MIXER,
        PHASE_TOTAL,
        PHASE_COUNT
    };

    static constexpr uint32_t PHASE_CONSUME_BUDGET_US = 150;
    static constexpr uint32_t PHASE_PID_BUDGET_US     = 650;
    static constexpr uint32_t PHASE_MIXER_BUDGET_US   = 300;
    static constexpr uint32_t PHASE_TOTAL_BUDGET_US   = 1500;

    static void init();           // Servo çıkışı + Core 1 başlatma
    static void core1_entry();    // PID + Mixer döngüsü (Core 1)

    static void initTimingMeasurements();
    static void beginTiming(LoopPhase phase);
    static void endTiming(LoopPhase phase);
    static bool checkTimingBudgets();
    static void logTimingStats();

    static volatile bool is_running;

    // --- Core 1 canlılık izleme (watchdog kör noktası düzeltmesi) ---
    // Core 1 her döngü turunda bu değeri günceller (armed/disarmed farketmeksizin).
    // Core 0, bu değeri okuyup taze mi diye kontrol eder ve ancak taze ise
    // watchdog_update() çağırır. Core 1 kilitlenirse besleme durur, donanım
    // watchdog'u WATCHDOG_TIMEOUT_MS içinde chip'i resetler.
    static volatile uint32_t core1HeartbeatUs;
    static uint32_t getCore1HeartbeatUs();
};
// Geriye uyumluluk için alias
using SystemManager = SystemTimer;

#endif