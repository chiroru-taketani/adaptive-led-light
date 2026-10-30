# Adaptive LED Light

Webカメラによる人物姿勢・作業モード認識と、64x32 RGB LEDマトリックスパネル（ESP32-LEDPRO-R2）を組み合わせた集中支援スマート面照明システム。

## ディレクトリ構成
- `firmware/`: ESP32-LEDPRO-R2 照明制御ファームウェア (PlatformIO / C++)
- `host/`: Webカメラ作業状態推定・シリアル送信プログラム (Python / OpenCV / MediaPipe)
- `docs/`: ハードウェア仕様・ピンアサイン・通信プロトコル
