#include "PreviewFrameBuffer.h"

#include <esp_heap_caps.h>

#include "../core/Logger.h"

namespace rvf {

bool PreviewFrameBuffer::attach(uint8_t* buffer, size_t capacity, PreviewFrameBufferStorage storage) {
    if (buffer == nullptr || capacity == 0) {
        reset();
        return false;
    }

    _buffer = buffer;
    _stats = PreviewFrameStats{};
    _stats.attached = true;
    _stats.storage = storage;
    _stats.capacity = capacity;
    captureMemoryStats();
    _lastStatsLogMs = 0;
    LOGI("FRAME", "buffer attached storage=%s cap=%u",
         storageName(),
         static_cast<unsigned>(capacity));
    return true;
}

void PreviewFrameBuffer::reset() {
    _buffer = nullptr;
    _stats = PreviewFrameStats{};
    _lastStatsLogMs = 0;
}

void PreviewFrameBuffer::resetRuntimeStats() {
    const bool wasAttached = _stats.attached;
    const PreviewFrameBufferStorage storage = _stats.storage;
    const size_t capacity = _stats.capacity;
    _stats = PreviewFrameStats{};
    _stats.attached = wasAttached;
    _stats.storage = storage;
    _stats.capacity = capacity;
    captureMemoryStats();
    _lastStatsLogMs = 0;
}

void PreviewFrameBuffer::recordFrame(size_t size) {
    _stats.renderedFrames++;
    _stats.lastFrameSize = size;
    if (size > _stats.maxFrameSize) {
        _stats.maxFrameSize = size;
    }
    const uint32_t cappedSize = size > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(size);
    const uint32_t remaining = UINT32_MAX - _stats.totalFrameBytes;
    _stats.totalFrameBytes += cappedSize > remaining ? remaining : cappedSize;
}

void PreviewFrameBuffer::syncStreamStats(uint32_t streamFrames, uint32_t droppedFrames, size_t currentLength) {
    _stats.streamFrames = streamFrames;
    _stats.droppedFrames = droppedFrames;
    _stats.currentFrameLength = currentLength;
}

void PreviewFrameBuffer::captureMemoryStats() {
    _stats.freeInternalBytes = static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    _stats.largestInternalBlockBytes = static_cast<uint32_t>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    _stats.freePsramBytes = static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void PreviewFrameBuffer::logStatsIfDue(uint32_t nowMs, uint32_t intervalMs) {
    if (!attached()) {
        return;
    }
    if (_lastStatsLogMs != 0 && (nowMs - _lastStatsLogMs) < intervalMs) {
        return;
    }

    _lastStatsLogMs = nowMs;
    captureMemoryStats();
    LOGI("FRAME",
         "buffer storage=%s cap=%u stream=%lu rendered=%lu dropped=%lu last=%u max=%u current=%u free_int=%lu largest_int=%lu free_psram=%lu",
         storageName(),
         static_cast<unsigned>(_stats.capacity),
         static_cast<unsigned long>(_stats.streamFrames),
         static_cast<unsigned long>(_stats.renderedFrames),
         static_cast<unsigned long>(_stats.droppedFrames),
         static_cast<unsigned>(_stats.lastFrameSize),
         static_cast<unsigned>(_stats.maxFrameSize),
         static_cast<unsigned>(_stats.currentFrameLength),
         static_cast<unsigned long>(_stats.freeInternalBytes),
         static_cast<unsigned long>(_stats.largestInternalBlockBytes),
         static_cast<unsigned long>(_stats.freePsramBytes));
}

const char* PreviewFrameBuffer::storageName() const {
    switch (_stats.storage) {
        case PreviewFrameBufferStorage::Psram:
            return "psram";
        case PreviewFrameBufferStorage::InternalRam:
            return "internal";
        case PreviewFrameBufferStorage::Unknown:
            return "unknown";
    }
    return "unknown";
}

}  // namespace rvf
