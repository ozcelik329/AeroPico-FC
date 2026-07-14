#include "ServiceCommandMailbox.h"

#include <string.h>

bool ServiceCommandMailbox::submit(const ServiceCommandRequest& request) {
    uint8_t expected = 0;
    if (!__atomic_compare_exchange_n(&_pendingReady, &expected, (uint8_t)1,
                                     false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        return false;
    }
    _pending = request;
    __atomic_store_n(&_pendingReady, (uint8_t)2, __ATOMIC_RELEASE);
    return true;
}

bool ServiceCommandMailbox::claim(ServiceCommandRequest& request) {
    uint8_t expected = 2;
    if (!__atomic_compare_exchange_n(&_pendingReady, &expected, (uint8_t)1,
                                     false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        return false;
    }
    request = _pending;
    _pending = {};
    __atomic_store_n(&_pendingReady, (uint8_t)0, __ATOMIC_RELEASE);
    return true;
}

void ServiceCommandMailbox::complete(uint16_t action, uint8_t result, const char* reason) {
    ServiceCommandCompletion completion = {};
    completion.action = action;
    completion.result = result;
    if (reason) {
        strncpy(completion.reason, reason, sizeof(completion.reason) - 1);
    }
    _completion = completion;
    __atomic_store_n(&_completionReady, (uint8_t)1, __ATOMIC_RELEASE);
}

bool ServiceCommandMailbox::takeCompletion(ServiceCommandCompletion& completion) {
    uint8_t expected = 1;
    if (!__atomic_compare_exchange_n(&_completionReady, &expected, (uint8_t)0,
                                     false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        return false;
    }
    completion = _completion;
    _completion = {};
    return true;
}
