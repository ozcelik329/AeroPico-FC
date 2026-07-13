#ifndef PIN_VALIDATION_H
#define PIN_VALIDATION_H

#include "board/Config.h"

namespace BoardPins {

constexpr bool isUart0RxPin(int pin) {
    return pin == 1 || pin == 13 || pin == 17 || pin == 29;
}

constexpr bool isUart1RxPin(int pin) {
    return pin == 5 || pin == 9 || pin == 21 || pin == 25;
}

#if SBUS_UART_INDEX == 0
static_assert(isUart0RxPin(PIN_SBUS_RX), "PIN_SBUS_RX is not a valid UART0 RX pin");
#elif SBUS_UART_INDEX == 1
static_assert(isUart1RxPin(PIN_SBUS_RX), "PIN_SBUS_RX is not a valid UART1 RX pin");
#else
static_assert(SBUS_UART_INDEX == 0 || SBUS_UART_INDEX == 1, "SBUS_UART_INDEX must be 0 or 1");
#endif

}  // namespace BoardPins

#endif
