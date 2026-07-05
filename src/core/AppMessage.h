#pragma once

#include <cstdint>
#include "AppEvent.h"

namespace rvf {

struct AppMessage {
    AppEventType type = AppEventType::None;
    uint32_t timestampMs = 0;
    int code = 0;
    const char* detail = nullptr;
};

}  // namespace rvf
