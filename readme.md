# xiaoOS
同じOSでどのデバイスでも動かせます。ArduinoからPCまで全て1つのOSです。
## 設計思想
OSはカスタムされるべきです。OSは最小限のエンジンのみを提供し，基本的に機能は提供しません。
全ての機能はアプリによって動作します。OSは待機とアプリ起動のみをboot.txt形式で実行します。
GUI描画，ドライバすらも全てアプリで機能します。1つの部分が使えなくなることを理由に全てを書き換えてはなりません。

## 技術的な概要
OS本体は `boot.txt` 起動テキストを読み，`exec` / `spawn` / `wait` だけを実行します。
アプリはCで書かれ，ターゲットごとにネイティブオブジェクトとしてリンクされます。
現在はESP32-C3,S3,x86-BIOS,x86_64-UEFI,AArch64-UEFIで動作を確認しています。

## IPCと引数
terminalは入力された1行を `xiao_exec_line()` で起動メッセージに変換します。
起動されたアプリは `env->ipc` または `xiao_argc()` / `xiao_argv()` で引数を受け取ります。
つまり `grep hello readme.txt` は，terminalからgrepアプリへ `argv = ["grep", "hello", "readme.txt"]` を送るIPCとして扱われます。

## ファイルと基本コマンド
`files/` の中身はビルド時に初期ファイルシステムとしてOSイメージに入ります。
空フォルダを含むフォルダ構造もそのまま保持されます。
除外したいファイルは `files/.xiaoignore` に書きます。`.xiaoignore` 自体はOSイメージには含めません。
`make` 時には初期ファイルシステムを毎回再生成するため，`files/` 内の追加・削除・移動が反映されます。

ファイルシステムはcoreに小さなヘルパーとして入り，実際の操作はアプリから呼び出します。
これにより，Arduino/ESPではRAM上の小さな可変FSとして，PC/QEMUでは同じAPIのまま動きます。

現在の基本コマンド:
- `pwd`
- `cd DIR`
- `ls [DIR]`: ディレクトリ一覧
- `ls -a`: app一覧
- `cat FILE...`
- `mkdir DIR...`
- `cp SRC DST`: ファイルまたはディレクトリをコピー
- `mv SRC DST`: ファイルまたはディレクトリを移動
- `rm PATH...`: ファイルまたはディレクトリを削除
- `grep PATTERN FILE...`
- `sed s/OLD/NEW/ FILE...`
- `top`: 実行中アプリと待機タスクを表示
- `df`: xiaoFSのRAM/ノード/ROM使用量を表示
- `free`: xiaoFS RAM，FSノード，タスク枠の空き状況を表示

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
