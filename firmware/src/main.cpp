#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <math.h>
#include <string.h>

#include "config.h"
namespace {

//作業モード種別
enum class LightingMode{
    kAway, //離席：消灯待機 
    kPc, //PC作業中：昼白色（約4500K）・均一寄りのアンビエント面発光
    kDesk,// 手元作業中：昼光色（約5800K）・下側を強めたタスク配光
};

//色の情報を入れた構造体
struct RgbColor
{
    float r;
    float g;
    float b;
};

//グラデーション用の構造体

struct GradientProfile
{
    RgbColor top;//中身は（r，g，b）
    RgbColor bottom;
};

MatrixPanel_I2S_DMA* g_display = nullptr;

//照明の状態管理用の変数（0で初期化）
LightingMode g_target_mode = LightingMode::kAway;
GradientProfile g_start_profile = {{0.0f, 0.0f, 0.0f},{0.0f,0.0f,0.0f}};
GradientProfile g_current_profile = {{0.0f, 0.0f, 0.0f},{0.0f,0.0f,0.0f}};
GradientProfile g_target_profile = {{0.0f, 0.0f, 0.0f},{0.0f,0.0f,0.0f}};

//トランディションの初めの情報を入れる変数（ms）
uint32_t g_transition_start_ms = 0;
uint32_t g_last_frame_ms = 0 ;

//シリアルポートから送られてくる文字用の設定
char g_rx_buf[config::kSerialRxBufferSize];//変数定義データサイズ
size_t g_rx_len = 0;//文字の長さを入れる
bool g_rx_overflow = false;//オーバフローした時のフラグ

//モードごとの表示の設定をする関数
GradientProfile GetPresetProfile(LightingMode mode){
    switch (mode)
    {
    case LightingMode::kPc:
    //
    return {
        {210.0f, 185.0f, 145.0f},// 上端：目に優しいやや控えめな温白色
        {230.0f, 205.0f, 165.0f},// 上端：目に優しいやや控えめな温白色
    };

    case LightingMode::kDesk:
    //
    return{
          {165.0f, 180.0f, 195.0f},  // 上端：眩しさを抑えたクールホワイト
          {255.0f, 255.0f, 255.0f},  // 下端：机の手元側をしっかり明るく照射
    };

    case LightingMode::kAway:
    default:
     return {
                {0.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 0.0f},
            };
    }
}

//現状のモードを文字列に変換
const char* ModeToString(LightingMode mode){
    switch (mode)
    {
    case LightingMode::kPc:

        return"PC";
    case LightingMode::kDesk:
            return "DESK";
    case LightingMode::kAway:
    default:
        return "AWAY";
    }
}

//線形補完式
float Lerp(float a, float b, float t) {return a +(b-a) * t;}

//RgbColor型の数値a,bを線形補完式を使って変換
RgbColor LerpColor(const RgbColor& a, const RgbColor& b, float t){
    return{
        Lerp(a.r,b.r,t),
        Lerp(a.g, b.g, t),
        Lerp(a.b, b.b, t),
    };
}

//浮動小数点で計算した値をunit8_tに四捨五入して変換
uint8_t ClampChannel(float v){
    if(v <= 0.0f) return 0;
    if(v >= 255.0f) return 255;
    return static_cast<uint8_t>(v + 0.5f);//四捨五入(+0.5f)している
}

//照明のモードを変更する関数
void SetLightingMode(LightingMode new_mode, uint32_t now_ms){
    //指定されたモードが今の照明と同じ状態なら何もしない
    if(new_mode == g_target_mode){
        return;
    }

    g_target_mode = new_mode;//ターゲットモードに目標のモードをセット
    g_start_profile = g_current_profile;//今の見た目をセット
    g_target_profile = GetPresetProfile(new_mode);//目的の色を決める
    g_transition_start_ms = now_ms;//ディレーションのために今の時間を保存

    //遷移しているモードを標準出力
    Serial.print("INFO: Transition to MODE:");
    Serial.println(ModeToString(new_mode));

}

//テキストのコマンドを読み取って照明を操作する
void HandleSerialCommand(const char* cmd, uint32_t now_ms){
    if(strcmp(cmd, "MODE:PC")== 0){
        SetLightingMode(LightingMode::kPc, now_ms);
    }else if (strcmp(cmd, "MODE:DESK") == 0) {
        SetLightingMode(LightingMode::kDesk, now_ms);
    }else if(strcmp(cmd,"MODE:AWAY") == 0){
        SetLightingMode(LightingMode::kAway, now_ms);
    }else if (strcmp(cmd, "STATUS") == 0) {
        Serial.print("STATUS: mode=");
        Serial.println(ModeToString(g_target_mode));
    } else if (cmd[0] != '\0') {
        Serial.print("WARN: Unknown command ignored: ");
        Serial.println(cmd);
    }
}

//一文字ずつ読み取って文字列バッファ　に代入
void PollSerial(uint32_t now_ms){
    while (Serial.available() > 0){
        const char c = static_cast<char>(Serial.read());
        if(c == '\r'){
            continue;
        }
        if(c == '\n'){
            if(g_rx_overflow){
                Serial.println("WARN: Serial RX buffer overflow, line discarded.");
                g_rx_overflow = false;
            }else {
                g_rx_buf[g_rx_len] = '\0';
                HandleSerialCommand(g_rx_buf, now_ms);
            }
            g_rx_len = 0;
            continue;
        }
        if (!g_rx_overflow) {
            if (g_rx_len + 1 < config::kSerialRxBufferSize) {
                g_rx_buf[g_rx_len++] = c;
            } else {
                g_rx_overflow = true;
            }
        }
    }
}

//現在の時間に応じたイージング補間と呼吸ゆらぎを計算し、64x32パネル全面を更新する
void UpdateAndRenderLighting(uint32_t now_ms){
    if(g_display == nullptr){
        return;
    }

    //moothstep イージング（t^2 * (3 - 2t)）による滑らかなクロスフェード進行度
    float progress = 1.0f;
    if(config::kTransitionDurationMs > 0){
        const uint32_t elapsed = now_ms- g_transition_start_ms;
        if(elapsed < config::kTransitionDurationMs){
            const float t =static_cast<float>(elapsed)/static_cast<float>(config::kTransitionDurationMs);
            progress = t * t * (3.0f - 2.0f * t);
        }
    }


    g_current_profile.top = LerpColor(g_start_profile.top, g_target_profile.top, progress);
    g_current_profile.bottom = LerpColor(g_start_profile.bottom, g_target_profile.bottom, progress);

    //点灯中は周辺視野に心地よい微細な呼吸ゆらぎ（±3%）を乗せる
    float breath_scale =1.0f;
    if(!(g_target_mode == LightingMode::kAway && progress >= 1.0f)){
        const float phase = (2.0f * static_cast<float>(M_PI) * static_cast<float>(now_ms)) /
            config::kBreathingPeriodMs;
        breath_scale = 1.0f + config::kBreathingAmplitude * sinf(phase);
    }

     // 縦32行ごとに上下グラデーション色を計算し、1行一括描画で高速に面発光を更新する
    for (uint16_t y = 0; y < config::kPanelResY; ++y) {
        const float row_ratio =
            static_cast<float>(y) / static_cast<float>(config::kPanelResY - 1);
        const RgbColor row_color =
            LerpColor(g_current_profile.top, g_current_profile.bottom, row_ratio);

        const uint8_t r = ClampChannel(row_color.r * breath_scale);
        const uint8_t g = ClampChannel(row_color.g * breath_scale);
        const uint8_t b = ClampChannel(row_color.b * breath_scale);

        g_display->drawFastHLine(
            0, y, config::kPanelResX, g_display->color565(r, g, b));
    }
}

}// namespace



void setup() {
    Serial.begin(config::kSerialBaudRate);

    // ESP32-LEDPRO-R2 のピン配置とクロック位相補正を設定
    HUB75_I2S_CFG::i2s_pins pins = {
        config::kPinR1, config::kPinG1, config::kPinB1,  config::kPinR2, config::kPinG2,
        config::kPinB2, config::kPinA,  config::kPinB,   config::kPinC,  config::kPinD,
        config::kPinE,  config::kPinLat, config::kPinOe, config::kPinClk};
    

    //mxconfig::マイコンの設定が入っている変数
    //横のドット数，縦のドット数，パネルを何枚連結しているか
    HUB75_I2S_CFG mxconfig(config::kPanelResX, config::kPanelResY, config::kPanelChain, pins);
    mxconfig.clkphase = config::kClkPhase;

     // パネル制御インスタンスの生成と初期化（失敗時はメモリ解放して安全停止）
     g_display = new MatrixPanel_I2S_DMA(mxconfig);
     if(g_display == nullptr || !g_display->begin()){
        Serial.println("ERROR: MatrixPanel_I2S_DMA initialization failed.");
        delete g_display;
        g_display = nullptr;
        return;
     }

     g_display->setBrightness8(config::ClampBrightness(config::kDefaultBrightness));
     g_display->clearScreen();

     // 起動時は消灯状態から PC作業モード（昼白色面発光）へふわっとフェードインさせる
    const uint32_t now_ms = millis();
    g_last_frame_ms = now_ms;
    SetLightingMode(LightingMode::kPc, now_ms);

    Serial.println("Adaptive LED Light Firmware - Ready (Send MODE:PC, MODE:DESK, MODE:AWAY)");

}
void loop() {
    const uint32_t now_ms = millis();
    PollSerial(now_ms);

    if (now_ms - g_last_frame_ms >= config::kFrameIntervalMs) {
        g_last_frame_ms = now_ms;
        UpdateAndRenderLighting(now_ms);
    }
}
