# xiaoOS
同じOSでどのデバイスでも動かせます。ArduinoからPCまで全て1つのOSです。
## 設計思想
OSはカスタムされるべきです。OSは最小限のエンジンのみを提供し，基本的に機能は提供しません。
全ての機能はアプリによって動作します。OSは待機とアプリ起動のみをboot.txt形式で実行します。
GUI描画，ドライバすらも全てアプリで機能します。OSのAPIを使用するのではなく，APIのようにアプリを使用します。OSは常にクリーンかつ最小限の機能を提供するだけに過ぎません。

## 技術的な概要
OS本体は `boot.txt` 起動テキストを読み，`exec` / `spawn` / `wait` だけを実行します。
アプリはCで書かれ，ターゲットごとにネイティブオブジェクトとしてリンクされます。
現在はESP32-C3,S3,x86-BIOS,x86_64-UEFI,AArch64-UEFIで動作を確認しています。

## 対応ターゲット
- Arduino Uno: `hal/arduino/xiaoOS/xiaoOS.ino`
- ESP32-C3: `hal/esp32c3/xiaoOS/xiaoOS.ino`
- x86 BIOS: `build/bios/xiao-bios.img`
- x86_64 UEFI: `build/uefi/BOOTX64.EFI`
- AArch64 UEFI: `build/arm64/BOOTAA64.EFI`

ArduinoにはUEFI/BIOSがないため，Arduinoではreset後にスケッチが直接 `xiao_start()` を呼びます。
PCではBIOS boot sectorまたはUEFI applicationが同じ `xiao_start()` を呼びます。
つまり「同じOS」とは，同じcore，同じapp API，同じboot.txt実行モデルを共有するという意味です。

## QEMU
- `./32.sh`: x86 BIOS。BIOS版は確認しやすさ優先で `-nographic` 起動です。
- `./64.sh`: x86_64 UEFI。QEMU画面にOS出力します。
- `./arm.sh`: AArch64 UEFI。QEMU virt向けにUSB/virtio keyboardを明示しています。
- `./rp.sh`: Raspberry Pi系プロファイル（QEMU raspi3b, Pi2クラス想定）で起動します。

## Linux / macOS 共通メモ
- `./64.sh` は `QEMU_EFI_CODE` を優先し，見つからない場合は `OVMF_CODE.fd` / `edk2-x86_64-code.fd` を主要ディレクトリから自動探索します。
- `./arm.sh` と `./rp.sh` は `QEMU_ACCEL` 未指定時，Apple Silicon macOSは `hvf`，Linux arm64/aarch64 で `/dev/kvm` があれば `kvm`，それ以外は `tcg` を使います。
- `QEMU_CPU` 未指定時，`kvm/hvf` では `host`，`tcg` では `max` を使います（`-cpu host` エラー回避）。
- AArch64 UEFIは `QEMU_EFI_CODE`（または `RPI_EFI_CODE`）で明示できます。自動探索は `QEMU_EFI.fd` / `AAVMF_CODE.fd` / `edk2-aarch64-code.fd` を対象にします。
- ESP32書き込みは `./esp32c3.sh [compile|upload|monitor|upload-monitor]`。Linuxでは `/dev/ttyUSB*` `/dev/ttyACM*` `/dev/serial/by-id/*` を自動探索します。
