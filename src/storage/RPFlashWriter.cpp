#include "RPFlashWriter.h"

#if !defined(UNIT_TEST)
#include <Arduino.h>
#endif

bool RPFlashWriter::execute(Mutation mutation, void* context) {
    if (!mutation) {
        return false;
    }

#if defined(UNIT_TEST)
    (void)context;
    return false;
#else
    // Arduino-Pico's FreeRTOS port provides a flash-safe cross-core gate.
    // It suspends this scheduler/core and parks the other core in RAM.
#if !defined(__FREERTOS)
    noInterrupts();
#endif
    rp2040.idleOtherCore();
    mutation(context);
    rp2040.resumeOtherCore();
#if !defined(__FREERTOS)
    interrupts();
#endif
    return true;
#endif
}
