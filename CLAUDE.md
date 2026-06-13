# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-S3-N16R8 development board driving a 128×160 RGB TFT (ST7735, 1.8″) via pure GPIO bit-banging — no SPI.h, no Adafruit library at runtime. The sketch is a single `.ino` file built with Arduino IDE.

## Hardware Pinout

| Pin | GPIO |
|-----|------|
| CS   | 13 |
| DC   | 9  |
| RST  | 10 |
| MOSI | 11 |
| SCLK | 12 |

Display resolution: 128×160, GREENTAB variant (XOFF=2, YOFF=1).

## Build & Upload

- **IDE**: Arduino IDE with ESP32 boards package 3.3.10
- **Board**: `ESP32S3 Dev Module` with Octal PSRAM **disabled** (QSPI mode works; OPI crashes)
- **Port**: `/dev/cu.usbmodem744DBD793EB41` or `/dev/cu.usbmodem14101` (device node changes on each physical reconnect)
- **Serial**: Both `Serial` and `USBSerial` work for USB CDC (115200 baud)

### Build cache

Before each compile, delete the stale build cache or the old binary gets re-uploaded:

```
rm -rf /Users/waiter/Library/Caches/arduino/sketches/3FA4585888169267365C6BE9A9625D56/
```

### Compiler for syntax checks

```
/Users/waiter/Library/Arduino15/packages/esp32/tools/esp-x32/2601/bin/xtensa-esp32s3-elf-g++ \
  -c -I/Users/waiter/Library/Arduino15/packages/esp32/hardware/esp32/3.3.10/cores/esp32 \
  -I/Users/waiter/Library/Arduino15/packages/esp32/hardware/esp32/3.3.10/variants/esp32s3 \
  -DARDUINO=10819 -DESP32 -DF_CPU=240000000L -DBOARD_HAS_PSRAM -std=gnu++17 \
  -fsyntax-only sketch_jun12a.ino
```

## Driver Architecture

The sketch contains a self-contained ST7735 driver in pure GPIO `digitalWrite()` — no SPI peripheral is used (SPI.h caused StoreProhibited crashes on this board with Arduino ESP32 3.3.10).

### SPI bit-banging layer

- `spiWrite(uint8_t b)` — manually toggles MOSI/SCLK for 8 bits, `delayMicroseconds(1)` between each edge
- `cmd(uint8_t)` / `dat(uint8_t)` / `dat16(uint16_t)` — set DC before CS, then write. **DC must be set BEFORE CS goes low** or the display misreads the first bit.

### Display init (`tftInit`)

Follows Adafruit ST7735R init sequences (Rcmd1 → Rcmd2 → Rcmd3):
- INVOFF (0x20), not INVON (0x21)
- COLMOD = 0x05 (16-bit/pixel RGB 5-6-5)
- Gamma correction tables (GMCTRP1 + GMCTRN1) from Adafruit library

### Critical protocol detail — CASET/RASET

`CASET` (0x2A) and `RASET` (0x2B) take **exactly 2** 16-bit parameters (start, end), never 4. Sending 4 parameters causes garbled output. Global ranges set during init: CASET(2, 129), RASET(1, 160).

### MADCTL (0x36) — Screen orientation

| Bit | Name | Effect |
|-----|------|--------|
| 7 (0x80) | MY | Row address decrement (bottom→top scan) |
| 6 (0x40) | MX | Column address decrement (right→left scan) |
| 5 (0x20) | MV | Row/column exchange (landscape/portrait) |
| 3 (0x08) | BGR | Swap red and blue channels |

**Current working value**: `0xC0` (MY=1, MX=1). This display has a permanent hardware left-right mirror that MX=1 compensates for. MY=1 is required for correct vertical orientation.

### Font system

- 64 glyphs (ASCII 32–95), 6 bytes per glyph, stored in PROGMEM
- Column-major: each byte = one column of the glyph, MSB = top pixel
- Access via `pgm_read_byte(&FONT[index][column])`
- Character cell: 6×8 pixels (before scaling)
- Lowercase letters are auto-converted to uppercase

### Rendering order (tied to MADCTL)

With MADCTL 0xC0 (MY=1, MX=1):
- Row loop: 7→0 (bottom-to-top of glyph, matching MY decrement)
- Column loop: 0→5 (left-to-right; MX=1 handles the scan direction flip)
- String output: normal left-to-right

## Key lessons from debugging

1. **SPI.h on ESP32-S3**: Causes StoreProhibited exception. Avoid — bit-bang instead.
2. **CASET/RASET parameter count**: Only 2 params each. The global init offset is separate from per-draw-call offsets.
3. **DC timing**: Must be set before CS goes low. All `cmd`/`dat`/`dat16` functions do this.
4. **MADCTL axis bits**: 0x40 is MX (bit 6), 0x80 is MY (bit 7). These are easily confused.
5. **Color order**: BGR bit (0x08) swaps red/blue. Must be 0 for correct RGB565 colors on this display.
6. **Build cache**: Forgetting to delete it before upload silently re-flashes the old binary.
7. **Always compile-check after edits** with `-fsyntax-only` before asking user to upload.
