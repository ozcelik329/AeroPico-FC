#ifndef BENCH_ADMIN_GATE_H
#define BENCH_ADMIN_GATE_H

#include <Arduino.h>

class BenchAdminGate {
  public:
    void init();
    bool forceArmActive() const;
};

extern BenchAdminGate benchAdminGate;

#endif
