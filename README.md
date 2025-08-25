# ILI9488 (320×480) on SAMD21 (Arduino Zero) inversion non-vivid fix & small shim.

This package contains everything you need to bring up some clone or cheap(ish/er) AE **320×480 SPI “ILI9488” TFT** on a **SAMD21 (Arduino/Genuino Zero)** with the exact settings we proved working for our panel / project maybe it will help you.

Symptoms, screen draws inverted, screen doesnt boot BL is on but black screen on, screen is fussy about settings.
Oddly enough some pins didn't work for me but D1,0,3 did work. No real logic as to why. But that's what you get for working with 2 clones :) (RobotDyn SAMD21 clone, ili9488 AE clone in my case)

**Why this works:** many ILI9488 breakouts need **18‑bit color (COLMOD=0x66)**, **display inversion ON**, and a **stronger power/gamma** init to avoid the washed/pastel “CMYK” look. The included raw driver enforces that and draws using **true 24‑bit writes** (3 bytes/pixel) so there’s no mismatch.

---

## Contents

- `src/ili9488_samd21_lockin.ino` — **RAW SPI driver (no libraries)**, ready to upload.
- `optional/Arduino_GFX_fix_shim.ino` — keep Arduino_GFX but apply a post‑begin() fixup.
- `pinmap.txt` — 14‑pin TFT header mapping → SAMD21 pins.
- `wiring_diagram.txt` — ASCII ICSP/SPI header and TFT mapping.
- `Troubleshooting.md` — quick fixes if you still see pastel/grey/blank.
- `LICENSE` — MIT.

---

## Quick Start (RAW SPI driver)

1. **Wire it** exactly per the pin map (below). **SPI uses the ICSP header**.
2. **Power:** logic at **3.3 V**. Feed **VDD + BL** with **3.3 V or 5 V**; if unstable/washed, use **5 V** for VDD+BL (common GND).
3. Open `src/ili9488_samd21_lockin.ino` in Arduino IDE.
4. Tools → Board: **Arduino/Genuino Zero (Native USB Port)**.
5. Upload. You should see RGB bars and a blinking magenta square.
6. If orientation is wrong, change `setRotation(0)` ↔ `setRotation(2)` and re‑upload.

### Known‑good orientations (MADCTL)

- `setRotation(0)` → MADCTL **0x48** (portrait 320×480) ✅
- `setRotation(2)` → MADCTL **0x88** (portrait flipped) ✅
- `setRotation(1)` → 0x28 (landscape), `setRotation(3)` → 0xE8 (landscape flipped)

*(All include the BGR bit; keep them as is to preserve correct color.)*

### SPI clock
- Start at **16 MHz** (`SPI_HZ` in the sketch).  
- If you see artifacts/flicker, drop to **8 MHz**.  
- Once solid, you can try **24 MHz**.

---

## Pin Map (TFT 14‑pin header → SAMD21)

```
1: VDD   → 3V3 (or 5V, see notes)
2: GND   → GND
3: CS    → D3
4: RST   → D1
5: DC    → D0
6: SDI   → ICSP MOSI
7: SCK   → ICSP SCK
8: BL    → 3V3 or 5V (if screen specc allows) (backlight, NOT a GPIO)
9: SDO   → ICSP MISO
10: TCK  → (touch SCK, unused here)
11: TCS  → (touch CS, unused here)
12: TDI  → (touch MOSI, unused here)
13: TDO  → (touch MISO, unused here)
14: PEN  → (touch IRQ, unused here)
```

**ICSP (SPI) header, top view (looking at the board):**
```
MISO | 3V3
SCK  | MOSI
RESET| GND
```

---

## Using Arduino_GFX (optional)

If you want higher‑level drawing APIs, use `optional/Arduino_GFX_fix_shim.ino`. It runs `gfx->begin()` and then applies the **18‑bit + INVON + VIVID** fix so the panel matches what Arduino_GFX draws. If your Arduino_GFX build still pushes 16‑bit to ILI9488, update the library or patch its ILI9488 init to `COLMOD=0x66` and add `INVON (0x21)`.

---

## Troubleshooting

- **Pastel/washed:** ensure `COLMOD=0x66`, `INVON`, and the VIVID sequence are applied; keep logic 3.3 V, try 5 V (if screen specc allows) on VDD+BL.
- **Blank/white after reacting:** lower `SPI_HZ` to 8 MHz; power‑cycle; check MOSI/MISO/SCK on **ICSP**.
- **Wrong orientation:** use `setRotation(0|1|2|3)`; 0 and 2 were confirmed good.
- **Flicker/tearing:** stay at 16 MHz; shorten wires; common ground.

Good luck — this bundle is the “known good” state we validated for the iLi9488 clone.
