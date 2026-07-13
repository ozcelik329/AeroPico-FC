#ifndef SEQLOCK_TOPIC_H
#define SEQLOCK_TOPIC_H

#include <stdint.h>
#include <type_traits>

template <typename T>
class SeqlockTopic {
    static_assert(std::is_trivially_copyable<T>::value,
                  "SeqlockTopic requires trivially copyable data");

  public:
    void publish(const T& value) {
        const uint32_t before = __atomic_load_n(&_sequence, __ATOMIC_RELAXED);
        __atomic_store_n(&_sequence, before + 1u, __ATOMIC_RELEASE);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        _value = value;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        __atomic_store_n(&_sequence, before + 2u, __ATOMIC_RELEASE);
        __atomic_store_n(&_published, true, __ATOMIC_RELEASE);
    }

    bool read(T& value) const {
        if (!__atomic_load_n(&_published, __ATOMIC_ACQUIRE)) {
            return false;
        }

        for (uint8_t attempt = 0; attempt < MAX_READ_ATTEMPTS; ++attempt) {
            const uint32_t before = __atomic_load_n(&_sequence, __ATOMIC_ACQUIRE);
            if (before & 1u) continue;
            __atomic_thread_fence(__ATOMIC_SEQ_CST);
            T snapshot = _value;
            __atomic_thread_fence(__ATOMIC_SEQ_CST);
            const uint32_t after = __atomic_load_n(&_sequence, __ATOMIC_ACQUIRE);
            if (before == after) {
                value = snapshot;
                return true;
            }
        }
        return false;
    }

    uint32_t sequence() const {
        return __atomic_load_n(&_sequence, __ATOMIC_ACQUIRE);
    }

  private:
    static constexpr uint8_t MAX_READ_ATTEMPTS = 8;
    alignas(4) mutable uint32_t _sequence = 0;
    alignas(4) mutable bool _published = false;
    T _value = {};
};

#endif
