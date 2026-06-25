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
- `./rp.sh qemu`: QEMU raspi3bエミュレータで起動します。

## Raspberry Pi 4
Raspberry Pi 4で動作させるにはUEFI対応ブートローダーが必要です。

### ブートチェーン
```
bootcode4.bin -> start4.elf -> u-boot.bin -> UEFI -> BOOTAA64.EFI
```

U-BootがUEFI互換を提供します (`CONFIG_EFI_LOADER`)。

### 必要なファームウェア
- `u-boot.bin` — U-Boot (UEFI対応ビルド)
- `start4.elf` — 標準RPi GPUファームウェア
- `fixup4.dat` — GPUファームウェアコンパニオン
- `bootcode4.bin` — RPiブートローダー (SPI EEPROMにも格納済み)

### 取得先
- U-Boot: https://ftp.denx.de/pub/u-boot/ または `apt install u-boot-rpi4`
- ビルド: `make rpi_4_defconfig && make`
- RPi firmware: https://github.com/raspberrypi/firmware/tree/master/boot

### イメージ生成
```sh
# U-Bootをダウンロード・ビルド後
RPI_UBOOT_DIR=/path/to/u-boot RPI_FIRMWARE_DIR=/path/to/rpi-firmware/boot ./rp.sh image

# SDカードに書き込み
sudo dd if=build/rpi4/xiaoOS-rpi4.img of=/dev/sdX bs=4M status=progress && sync
```