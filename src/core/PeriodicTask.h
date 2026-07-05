#pragma once

#include <cstdint>

namespace rvf {

class PeriodicTask {
public:
    explicit PeriodicTask(uint32_t intervalMs = 0) : _intervalMs(intervalMs) {}
    void setInterval(uint32_t intervalMs) { _intervalMs = intervalMs; }
    void reset(uint32_t nowMs) { _lastRunMs = nowMs; }
    bool due(uint32_t nowMs) const {
        return _intervalMs > 0 && static_cast<uint32_t>(nowMs - _lastRunMs) >= _intervalMs;
    }
    bool consumeIfDue(uint32_t nowMs) {
        if (!due(nowMs)) { return false; }
        _lastRunMs = nowMs;
        return true;
    }
private:
    uint32_t _intervalMs = 0;
    uint32_t _lastRunMs = 0;
};

}  // namespace rvf
