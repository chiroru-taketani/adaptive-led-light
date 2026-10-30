#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config.h"
namespace {
MatrixPanel_I2S_DMA* g_display = nullptr;
}  // namespace
void setup() {
    Serial.begin(config::kSerialBaudRate);
    HUB75_I2S_CFG mxconfig(
        config::kPanelResX,
        config::kPanelResY,
        config::kPanelChain);
    g_display = new MatrixPanel_I2S_DMA(mxconfig);
    if (g_display == nullptr || !g_display->begin()) {
        Serial.println("ERROR: MatrixPanel_I2S_DMA initialization failed.");
        g_display = nullptr;
        return;
    }
    g_display->setBrightness8(config::ClampBrightness(config::kDefaultBrightness));
    g_display->clearScreen();
    Serial.println("Adaptive LED Light Firmware - Ready");
}
void loop() {
    delay(10);
}
