#ifndef RUNNING_MEDIAN_H
#define RUNNING_MEDIAN_H

#include <stddef.h>

template <typename T, size_t WindowSize>
class RunningMedian {
    static_assert(WindowSize > 0, "WindowSize must be greater than zero");
    static_assert((WindowSize % 2) == 1, "WindowSize should be odd for a stable median");

  public:
    void reset() {
        _count = 0;
        _next = 0;
    }

    T update(T value) {
        push(value);
        return median();
    }

    void push(T value) {
        _values[_next] = value;
        _next = (_next + 1) % WindowSize;
        if (_count < WindowSize) {
            _count++;
        }
    }

    T median() const {
        if (_count == 0) {
            return T{};
        }

        T sorted[WindowSize];
        for (size_t i = 0; i < _count; i++) {
            sorted[i] = _values[i];
        }

        for (size_t i = 1; i < _count; i++) {
            T key = sorted[i];
            size_t j = i;
            while (j > 0 && key < sorted[j - 1]) {
                sorted[j] = sorted[j - 1];
                j--;
            }
            sorted[j] = key;
        }

        return sorted[(_count - 1) / 2];
    }

    size_t size() const {
        return _count;
    }

    bool isFull() const {
        return _count == WindowSize;
    }

  private:
    T _values[WindowSize] = {};
    size_t _count = 0;
    size_t _next = 0;
};

#endif
