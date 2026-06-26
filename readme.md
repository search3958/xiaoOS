# xiaoOS
同じOSでどのデバイスでも動かせます。ArduinoからPCまで全て1つのOSです。
## 設計思想
OSはカスタムされるべきです。OSは最小限のエンジンのみを提供し，基本的に機能は提供しません。
全ての機能はアプリによって動作します。OSは待機とアプリ起動のみをboot.txt形式で実行します。
GUI描画，ドライバすらも全てアプリで機能します。OSのAPIを使用するのではなく，APIのようにアプリを使用します。OSは常にクリーンかつ最小限の機能を提供するだけに過ぎません。

## 技術的な概要
OS本体は `boot.txt` 起動テキストを読み，`exec` / `spawn` / `wait` だけを実行します。
アプリはCで書かれ，ターゲットごとにネイティブオブジェクトとしてリンクされます。
現在はESP32-C3,S3,x86-UEFI,x86_64-UEFI,AArch64-UEFIで動作を確認しています。

## 対応ターゲット
- Arduino Uno: `hal/arduino/xiaoOS/xiaoOS.ino`
- ESP32-C3: `hal/esp32c3/xiaoOS/xiaoOS.ino`
- x86 BIOS: `build/bios/xiao-bios.img`
- x86_64 UEFI: `build/uefi/BOOTX64.EFI`
- AArch64 UEFI: `build/arm64/BOOTAA64.EFI`
- Raspberry Pi 4: `./rp.sh image` → `build/rpi4/xiaoOS-rpi4.img`

ArduinoにはUEFI/BIOSがないため，Arduinoではreset後にスケッチが直接 `xiao_start()` を呼びます。
PCではBIOS boot sectorまたはUEFI applicationが同じ `xiao_start()` を呼びます。
つまり「同じOS」とは，同じcore，同じapp API，同じboot.txt実行モデルを共有するという意味です。

## QEMU
- `./32.sh`: x86 UEFI。QEMU画面にOS出力します。
- `./64.sh`: x86_64 UEFI。QEMU画面にOS出力します。
- `./arm.sh`: AArch64 UEFI。QEMU virt向けにUSB/virtio keyboardを明示しています。
- `./rp.sh image`: Raspberry Pi 4向けのブータブルSDカードイメージを生成します。
- `./rp.sh qemu`: QEMU raspi4bエミュレータで起動します。

## Raspberry Pi 4
Raspberry Pi 4で動作させるにはUEFI対応ブートローダーが必要です。

### ブートチェーン
```
EEPROM -> start4.elf -> kernel8.img (U-Boot) -> UEFI -> BOOTAA64.EFI
```

U-BootがUEFI互換を提供します (`CONFIG_EFI_LOADER`)。

### 必要なファームウェア
- `kernel8.img` — U-Boot (UEFI対応ビルド、RPi 4はkernel8.imgとして配置)
- `start4.elf` — 標準RPi GPUファームウェア
- `fixup4.dat` — GPUファームウェアコンパニオン

### 取得先
- U-Boot: `git clone https://source.denx.de/u-boot/u-boot.git && make rpi_4_defconfig && make`
- RPi firmware: https://github.com/raspberrypi/firmware/tree/master/boot

### 前提条件: EEPROM更新
**USBブートには最新のEEPROMが必要です。** 古いEEPROMではUSBから起動しません。
```sh
# 別のRPi 4から実行
sudo rpi-eeprom-update -a
sudo reboot
```
またはRaspberry Pi ImagerでEEPROMを更新。

### イメージ生成
```sh
# 自動的にU-BootとRPiファームウェアを探してイメージ生成
./rp.sh image

# SDカード/USBに書き込み
sudo dd if=build/rpi4/xiaoOS-rpi4.img of=/dev/sdX bs=4M status=progress && sync

# QEMUで起動確認
./rp.sh qemu
```