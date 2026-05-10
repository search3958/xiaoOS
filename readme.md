# xiaoOS
同じOSでどのデバイスでも動かせます。ArduinoからPCまで全て1つのOSです。
## 設計思想
OSはカスタムされるべきです。OSは最小限のエンジンのみを提供し，基本的に機能は提供しません。
全ての機能はアプリによって動作します。OSは待機とアプリ起動のみをboot.txt形式で実行します。
GUI描画，ドライバすらも全てアプリで機能します。1つの部分が使えなくなることを理由に全てを書き換えてはなりません。

## 現在の実装
最初の実装では，OS本体は `boot.txt` 相当の起動テキストを読み，`exec` / `spawn` / `wait` だけを実行します。
hello worldもシリアル出力もアプリです。アプリはCで書かれ，ターゲットごとにネイティブオブジェクトとしてリンクされます。

いまは外部ストレージを使わないため，`boot/common/boot.txt` と `apps/` から起動イメージを生成して埋め込んでいます。
生成されたapp tableは `build/generated/image.c` に出力されます。
将来ストレージを追加する場合も，この `xiao_boot_image` をディスクやフラッシュから読むloaderへ差し替えるだけにします。

現在のboot.txt:

```sh
exec hello
wait 100
exec serial_hello
exec nihao
wait forever
```

## 対応ターゲット
- Arduino Uno: `hal/arduino/xiaoOS/xiaoOS.ino`
- ESP32-C3: `hal/esp32c3/xiaoOS/xiaoOS.ino`
- x86 BIOS: `build/bios/xiao-bios.img`
- x86_64 UEFI: `build/uefi/BOOTX64.EFI`
- AArch64 UEFI: `build/arm64/BOOTAA64.EFI`

ArduinoにはUEFI/BIOSがないため，Arduinoではreset後にスケッチが直接 `xiao_start()` を呼びます。
PCではBIOS boot sectorまたはUEFI applicationが同じ `xiao_start()` を呼びます。
つまり「同じOS」とは，同じcore，同じapp API，同じboot.txt実行モデルを共有するという意味です。

## ビルド
PC向けBIOS/UEFIを同時にビルド:

```sh
make check
```

Arduino Uno向け:

```sh
arduino-cli core install arduino:avr
make arduino
```

Arduino IDEを使う場合は `hal/arduino/xiaoOS/xiaoOS.ino` を開いてArduino Uno向けにビルドします。
`apps/` や `boot.txt` を変更した後は，先に `make sync-sketches` でスケッチ内の生成ファイルを更新します。

ESP32-C3向け:

```sh
brew install arduino-cli
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
./esp32c3.sh
```

`esp32c3.sh` は `/dev/cu.usbmodem*` などを自動検出して，ビルド，書き込み，シリアルモニタを行います。
このスクリプトは書き込み前にESP32-C3スケッチ内の生成ファイルも更新します。
ポートを明示する場合:

```sh
PORT=/dev/cu.usbmodem1101 ./esp32c3.sh
```

USB CDCを使うため，標準FQBNは `esp32:esp32:esp32c3:CDCOnBoot=cdc` です。
ボード定義によってoption名が違う場合は `FQBN=... ./esp32c3.sh` で上書きします。

## アプリ
`apps/` の `*.c` は自動で全てappとして含まれます。
app名はファイル名から決まります。たとえば `apps/serial_hello.c` は `boot.txt` から `exec serial_hello` で起動できます。

各appは `xiao_app_entry()` を実装します。

```c
#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    xiao_serial_print(env, "hello from app\r\n");
    return 0;
}
```

除外したいappは `apps/.xiaoignore` にglobで書きます。
各appはビルド時に `tools/wrap_app.py` でapp名prefix付きの内部シンボルへ変換されます。
そのため，複数appが同じ名前のヘルパー関数を持っていてもリンク衝突しません。

## QEMU
macOS/HomebrewのQEMUなら，以下のスクリプトで起動できます。
QEMUの画面ウィンドウを開き，serial出力もQEMU側の仮想コンソールへ送ります。
PC HALではserial出力を画面にもミラーするので，`serial_hello` もQEMU画面で確認できます。

```sh
./32.sh   # x86 BIOS
./64.sh   # x86_64 UEFI
./arm.sh  # AArch64 UEFI
```

UEFI firmwareの場所が自動検出できない場合は，`QEMU_EFI_CODE` に指定します。

```sh
QEMU_EFI_CODE=/path/to/edk2-x86_64-code.fd ./64.sh
QEMU_EFI_CODE=/path/to/edk2-aarch64-code.fd ./arm.sh
```

## アプリが消えても起動する仕組み
app tableは `apps/` の実在ファイルから生成されます。
`boot.txt` に存在しないapp名が書かれていても，OSはその行をスキップして起動を続けます。

## まだ曖昧な点
- 将来，別アーキテクチャ間で同一appバイナリにする場合は，Cネイティブだけでは不可能です。まずは同じCソース，同じAPI，ターゲット別ネイティブビルドで進めます。
- Arduino Unoは外部ストレージなしなので，実行ファイルを本当にファイルとして保持するにはSDカードや外部フラッシュが必要です。現段階ではファームウェア内のapp tableを実行ファイル置き場として扱います。
- マルチタスクは全ターゲット共通で協調的です。`wait` / `yield` / `spawn` を基礎にし，プリエンプティブ切り替えはまだ入れていません。
