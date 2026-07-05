#include "SystemSupervisor.h"

#include <limits.h>

namespace rvf {

namespace {

bool isPreviewState(AppState state) {
    return state == AppState::LiveViewRunning || state == AppState::PreviewRunning;
}

uint32_t elapsedSince(uint32_t nowMs, uint32_t timestampMs) {
    return (timestampMs - nowMs) < 0x80000000UL ? 0 : nowMs - timestampMs;
}

bool timestampAfterOrEqual(uint32_t candidateMs, uint32_t currentMs) {
    return (candidateMs - currentMs) < 0x80000000UL;
}

AppMessage makeMessage(AppEventType type, uint32_t nowMs, int code, const char* detail) {
    AppMessage message;
    message.type = type;
    message.timestampMs = nowMs;
    message.code = code;
    message.detail = detail;
    return message;
}

}  // namespace

void SystemSupervisor::begin(uint32_t nowMs) {
    _checkTask.reset(nowMs);
}

bool SystemSupervisor::check(uint32_t nowMs,
                             const SystemHealthSnapshot& snapshot,
                             AppMessage& outMessage) {
    if (!_checkTask.consumeIfDue(nowMs)) {
        return false;
    }

    outMessage = AppMessage{};
    if (snapshot.cameraSleepGuardActive || snapshot.cameraRecoveryInProgress) {
        return false;
    }

    if (!isPreviewState(snapshot.appState) || !snapshot.liveviewEnabled) {
        return false;
    }

    if (!snapshot.bleConnected) {
        outMessage = makeMessage(AppEventType::BleDisconnected, nowMs, 0, "supervisor preview lost BLE");
        return true;
    }
    if (!snapshot.wifiConnected) {
        outMessage = makeMessage(AppEventType::WifiDisconnected, nowMs, 0, "supervisor preview lost WiFi");
        return true;
    }
    if (!snapshot.previewRunning) {
        outMessage = makeMessage(AppEventType::PreviewStopped, nowMs, 0, "supervisor preview closed");
        return true;
    }

    uint32_t lastActivityAt = snapshot.lastFrameAt;
    if (timestampAfterOrEqual(snapshot.lastLiveViewActivityAt, lastActivityAt)) {
        lastActivityAt = snapshot.lastLiveViewActivityAt;
    }

    const uint32_t idleMs = elapsedSince(nowMs, lastActivityAt);
    if (snapshot.liveViewStallTimeoutMs > 0 && idleMs > snapshot.liveViewStallTimeoutMs) {
        const int code = idleMs > static_cast<uint32_t>(INT_MAX) ? INT_MAX : static_cast<int>(idleMs);
        outMessage = makeMessage(AppEventType::PreviewTimeout, nowMs, code, "supervisor preview idle");
        return true;
    }

    return false;
}

}  // namespace rvf
