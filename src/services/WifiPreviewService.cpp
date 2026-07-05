#include "WifiPreviewService.h"

#include "../core/Logger.h"

namespace rvf {

void WifiPreviewService::attach(GrWifi& wifi, GrApi& api, MjpegStream& stream) {
    _wifi = &wifi;
    _api = &api;
    _stream = &stream;
    _lastError = "";
}

bool WifiPreviewService::attached() const {
    return _wifi != nullptr && _api != nullptr && _stream != nullptr;
}

void WifiPreviewService::setEndpoint(const char* host, uint16_t port) {
    if (_api != nullptr) {
        _api->setEndpoint(host, port);
    }
}

Result WifiPreviewService::activateWifi() {
    return requireAttached("activateWifi");
}

Result WifiPreviewService::connectWifi(const char* ssid,
                                       const char* password,
                                       const char* bssid,
                                       uint8_t channel,
                                       uint32_t timeoutMs,
                                       GrWifi::ConnectGuard guard) {
    Result ready = requireAttached("connectWifi");
    if (ready.failed()) {
        return ready;
    }

    const uint32_t startedAt = millis();
    const bool connected = (guard != nullptr)
                             ? _wifi->connect(ssid, password, bssid, channel, timeoutMs, guard)
                             : _wifi->connect(ssid, password, bssid, channel, timeoutMs);
    const uint32_t elapsed = millis() - startedAt;
    if (!connected) {
        setError(String("WiFi connect failed status=") + _wifi->statusText());
        LOGW("WIFI", "connect failed in %lums status=%s",
             static_cast<unsigned long>(elapsed),
             _wifi->statusText().c_str());
        return Result::failure(ErrorCode::WifiConnectFailed, _lastError);
    }

    _lastError = "";
    LOGI("WIFI", "connected in %lums ip=%s rssi=%ld",
         static_cast<unsigned long>(elapsed),
         _wifi->localIPString().c_str(),
         static_cast<long>(_wifi->rssi()));
    return Result::success();
}

void WifiPreviewService::disconnectWifi() {
    if (_api != nullptr) {
        _api->closeLiveView();
    }
    if (_stream != nullptr) {
        _stream->reset();
    }
    if (_wifi != nullptr) {
        _wifi->disconnect();
    }
}

bool WifiPreviewService::isWifiConnected() const {
    return _wifi != nullptr && _wifi->isConnected();
}

Result WifiPreviewService::fetchProps(CameraProps& props, uint32_t timeoutMs) {
    Result ready = requireAttached("fetchProps");
    if (ready.failed()) {
        props = CameraProps{};
        return ready;
    }

    const uint32_t startedAt = millis();
    if (!_api->fetchProps(props, timeoutMs)) {
        setError(_api->lastError());
        return Result::failure(ErrorCode::HttpProbeFailed, _lastError);
    }

    LOGI("HTTP", "props fetched in %lums",
         static_cast<unsigned long>(millis() - startedAt));
    _lastError = "";
    return Result::success();
}

Result WifiPreviewService::startPreview() {
    Result ready = requireAttached("startPreview");
    if (ready.failed()) {
        return ready;
    }

    const uint32_t startedAt = millis();
    if (!_api->openLiveView()) {
        setError(_api->lastError());
        return Result::failure(ErrorCode::PreviewTimeout, _lastError);
    }

    _stream->reset();
    resetStats(millis());
    LOGI("PREVIEW", "liveview opened in %lums",
         static_cast<unsigned long>(millis() - startedAt));
    _lastError = "";
    return Result::success();
}

void WifiPreviewService::stopPreview() {
    if (_api != nullptr) {
        _api->closeLiveView();
    }
    if (_stream != nullptr) {
        _stream->reset();
    }
}

bool WifiPreviewService::isPreviewRunning() const {
    return _api != nullptr && const_cast<GrApi*>(_api)->isLiveViewOpen();
}

int WifiPreviewService::readFrame(uint8_t* dst, size_t len) {
    if (_api == nullptr) {
        setError("WiFi preview service not attached: readFrame");
        return -1;
    }

    const uint32_t startedAt = millis();
    const int readLen = _api->readLiveView(dst, len);
    _lastReadMs = millis() - startedAt;
    if (readLen < 0) {
        setError(_api->lastError());
    } else if (readLen > 0) {
        _bytesInWindow += static_cast<uint32_t>(readLen);
        _lastError = "";
    }
    return readLen;
}

size_t WifiPreviewService::processFrameData(const uint8_t* data, size_t len) {
    if (_stream == nullptr) {
        setError("WiFi preview service not attached: processFrameData");
        return 0;
    }

    const uint32_t startedAt = millis();
    const size_t consumed = _stream->process(data, len);
    _lastProcessMs = millis() - startedAt;
    return consumed;
}

void WifiPreviewService::resetStats(uint32_t nowMs) {
    const uint32_t now = nowMs != 0 ? nowMs : millis();
    _statsWindowStartMs = now;
    _lastStatsLogMs = now;
    _lastReadMs = 0;
    _lastProcessMs = 0;
    _lastDecodeMs = 0;
    _lastRenderMs = 0;
    _framesInWindow = 0;
    _bytesInWindow = 0;
}

void WifiPreviewService::recordRenderedFrame(uint32_t decodeMs, uint32_t renderMs) {
    _lastDecodeMs = decodeMs;
    _lastRenderMs = renderMs;
    _framesInWindow++;
}

void WifiPreviewService::logStatsIfDue(uint32_t nowMs, uint32_t intervalMs) {
    if (_statsWindowStartMs == 0) {
        resetStats(nowMs);
        return;
    }
    if ((nowMs - _lastStatsLogMs) < intervalMs) {
        return;
    }

    const uint32_t elapsed = nowMs - _statsWindowStartMs;
    const float fps = elapsed > 0 ? (_framesInWindow * 1000.0f) / static_cast<float>(elapsed) : 0.0f;
    LOGI("PREVIEW", "stats fps=%.1f bytes=%lu read=%lums mjpeg_cb=%lums decode=%lums render=%lums",
         static_cast<double>(fps),
         static_cast<unsigned long>(_bytesInWindow),
         static_cast<unsigned long>(_lastReadMs),
         static_cast<unsigned long>(_lastProcessMs),
         static_cast<unsigned long>(_lastDecodeMs),
         static_cast<unsigned long>(_lastRenderMs));
    resetStats(nowMs);
}

Result WifiPreviewService::requireAttached(const char* operation) const {
    if (attached()) {
        return Result::success();
    }

    String message("WiFi preview service not attached");
    if (operation != nullptr && operation[0] != '\0') {
        message += ": ";
        message += operation;
    }
    return Result::failure(ErrorCode::InvalidState, message);
}

void WifiPreviewService::setError(const String& message) {
    _lastError = message;
}

}  // namespace rvf
