# xiaoOS
同じOSでどのデバイスでも動かせます。ArduinoからPCまで全て1つのOSです。
## 設計思想
OSはカスタムされるべきです。OSは最小限のエンジンのみを提供し，基本的に機能は提供しません。
全ての機能はアプリによって動作します。OSは待機とアプリ起動のみをboot.txt形式で実行します。
GUI描画，ドライバすらも全てアプリで機能します。OSは待機とアプリ起動のみをboot.txt形式で実行します。OSは常にクリーンかつ最小限の機能を提供するだけに過ぎません。

## 技術的な概要
OS本体は `boot.txt` 起動テキストを読み，`exec` / `spawn` / `wait` だけを実行します。
アプリはCで書かれ，ターゲットごとにネイティブオブジェクトとしてリンクされます。
現在はESP32-C3,S3,x86-BIOS,x86_64-GRUB(Multiboot2),AArch64-UEFIで動作を確認しています。

## 対応ターゲット
- Arduino Uno: `hal/arduino/xiaoOS/xiaoOS.ino`
- ESP32-C3: `hal/esp32c3/xiaoOS/xiaoOS.ino`
- PC (GRUB): `build/xiaoOS-grub.iso` (Multiboot2)

PC版（x86_64）はGRUBブートローダーに移行しました。
ビルドには `clang`, `lld`, `nasm`, `grub-mkrescue`, `xorriso` が必要です。

## QEMU
- `./32.sh`: x86_64 GRUB (BIOS)。
- `./64.sh`: x86_64 GRUB (UEFI)。
- `./arm.sh`: AArch64 UEFI。QEMU virt向けにUSB/virtio keyboardを明示しています。
- `./rp.sh`: Raspberry Pi系プロファイル（QEMU raspi3b, Pi2クラス想定）で起動します。
