"""Adaptive LED Light - Host Application Entry Point."""
import cv2
import mediapipe as mp
import serial
def main() -> None:
    print("Adaptive LED Light Host - Environment Ready")
    print(f"  OpenCV   : {cv2.__version__}")
    print(f"  MediaPipe: {mp.__version__}")
    print(f"  pyserial : {serial.__version__}")
if __name__ == "__main__":
    main()
