#pragma once
#include <stdint.h>
namespace config {
// P4 RGB LED Matrix Panel (64x32, single panel)
constexpr uint16_t kPanelResX = 64;
constexpr uint16_t kPanelResY = 32;
constexpr uint16_t kPanelChain = 1;
// Serial communication (must match docs/protocol.md)
constexpr uint32_t kSerialBaudRate = 115200;
// Hardware safety: cap max brightness to 50% (128/255) to prevent overcurrent/overheating
constexpr uint8_t kMaxSafeBrightness = 128;
constexpr uint8_t kDefaultBrightness = 64;
inline uint8_t ClampBrightness(uint8_t requested) {
    return (requested > kMaxSafeBrightness) ? kMaxSafeBrightness : requested;
}
}  // namespace config
