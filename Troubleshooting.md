# Troubleshooting

- **Pastel / “CMYK” look**
  - Panel isn’t actually in 18‑bit, inversion/gamma not set, or library sent 16‑bit data.
  - Fix: ensure `COLMOD=0x66`, send **VIVID** power/gamma sequence, `INVON (0x21)`.
- **Blank or goes white/grey after reacting**
  - Try `SPI_HZ = 8000000`.
  - Power cycle the panel.
  - Verify MOSI/MISO/SCK are from **ICSP**, not D11–D13.
  - Feed **VDD + BL** from a solid rail (often **5 V** helps), logic stays **3.3 V**.
- **Wrong orientation**
  - Use `setRotation(0|1|2|3)`. Known good for this panel: rot 0 (MADCTL 0x48) and rot 2 (0x88).
- **Flicker / tearing / random pixels**
  - Keep wires short/twisted.
  - Stay at 16 MHz until stable, then try 24 MHz.
  - Ensure common ground and decent bulk cap on the TFT rail (e.g., 47–100 µF).
