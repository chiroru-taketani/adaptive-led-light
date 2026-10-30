#pragma once
#include <stdint.h>
namespace config {
// P4 RGB LED Matrix Panel (64x32, single panel)
constexpr uint16_t kPanelResX = 64;
constexpr uint16_t kPanelResY = 32;
constexpr uint16_t kPanelChain = 1;

// クロック位相設定（FM6124系ドライバでの右端1列回り込みを防ぐため false に固定）
constexpr bool kClkPhase = false;

 // ESP32-LEDPRO-R2 の HUB75 GPIOピン割り当て
constexpr int8_t kPinR1 = 25;   // 上段 赤
constexpr int8_t kPinG1 = 26;   // 上段 緑
constexpr int8_t kPinB1 = 27;   // 上段 青
constexpr int8_t kPinR2 = 14;   // 下段 赤
constexpr int8_t kPinG2 = 12;   // 下段 緑
constexpr int8_t kPinB2 = 13;   // 下段 青
constexpr int8_t kPinA = 23;    // 行選択アドレス A
constexpr int8_t kPinB = 19;    // 行選択アドレス B
constexpr int8_t kPinC = 5;     // 行選択アドレス C
constexpr int8_t kPinD = 17;    // 行選択アドレス D
constexpr int8_t kPinE = -1;    // 行選択アドレス E（縦32ドットパネルでは未使用）
constexpr int8_t kPinLat = 4;   // ラッチ信号
constexpr int8_t kPinOe = 15;   // アウトプットイネーブル（輝度制御）
constexpr int8_t kPinClk = 16;  // クロック信号



  // ESP32-LEDPRO-R2 オンボードセンサーのピン割り当て（基板シルク印字より確認）
  constexpr uint8_t kPinPhotoSensor = 36;  // 明るさセンサー（PHOTO TR: D36 / ADC1_CH0）
  constexpr uint8_t kPinI2cSda = 21;       // I2C SDA（温湿度 AHT20・加速度 KXTJ3-1057 共通）
  constexpr uint8_t kPinI2cScl = 22;       // I2C SCL

  // シリアル通信設定（docs/protocol.md と一致させる）
  constexpr uint32_t kSerialBaudRate = 115200;
  constexpr size_t kSerialRxBufferSize = 32;

  // ハードウェア安全設計：過電流・発熱防止のため最大輝度を50%（128/255）に制限
  constexpr uint8_t kMaxSafeBrightness = 128;
  constexpr uint8_t kDefaultBrightness = 96;

  // 照明アニメーション設定
 constexpr uint32_t kTransitionDurationMs = 2000;  // モード切替クロスフェード時間（ミリ秒）
 constexpr uint32_t kFrameIntervalMs = 20;         // 描画更新間隔（約50FPS）
 constexpr float kBreathingAmplitude = 0.03f;      // 集中を妨げない微細な呼吸ゆらぎ振幅（±3%）
 constexpr float kBreathingPeriodMs = 8000.0f;     // 呼吸ゆらぎ周期（8秒）

  // 指定された輝度値を安全上限以内に丸める関数
  inline uint8_t ClampBrightness(uint8_t requested) {
      return (requested > kMaxSafeBrightness) ? kMaxSafeBrightness : requested;
  }

  }  // namespace config
