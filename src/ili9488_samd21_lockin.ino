// SAMD21 (Arduino Zero) + ILI9488 — RAW SPI driver with rotation (no libraries)
// Pins: CS=D3, DC=D0, RST=D1; MOSI/MISO/SCK on ICSP; BL to rail (NOT a GPIO).
// Rotation mapping (MADCTL): rot0=0x48, rot1=0x28, rot2=0x88, rot3=0xE8.
// Confirmed GOOD on this panel: 0x48 (rot 0) and 0x88 (rot 2).

#include <Arduino.h>
#include <SPI.h>

// ---- Pins ----
#define PIN_TFT_CS   3
#define PIN_TFT_DC   0
#define PIN_TFT_RST  1

// ---- SPI ----
static const uint32_t SPI_HZ = 16000000UL; // start 16 MHz; drop to 8000000 if flaky; push to 24000000 if solid

// ---- Panel geometry (swaps on rot 1/3) ----
static uint16_t g_w = 320, g_h = 480;

// ---- Low-level SPI helpers ----
static inline void csLow(){  digitalWrite(PIN_TFT_CS, LOW); }
static inline void csHigh(){ digitalWrite(PIN_TFT_CS, HIGH); }
static inline void dcCmd(){  digitalWrite(PIN_TFT_DC, LOW); }
static inline void dcData(){ digitalWrite(PIN_TFT_DC, HIGH); }
static inline void wr8(uint8_t d){ SPI.transfer(d); }

static void cmd(uint8_t c){ csLow(); dcCmd(); wr8(c); csHigh(); }
static void data1(uint8_t d){ csLow(); dcData(); wr8(d); csHigh(); }

static void writeSeq(const uint8_t *seq, size_t n){
  size_t i=0;
  while (i<n){
    uint8_t reg = seq[i++], cnt = seq[i++];
    csLow(); dcCmd(); wr8(reg); csHigh();
    if (cnt){ csLow(); dcData(); while(cnt--) wr8(seq[i++]); csHigh(); }
  }
}

static void setAddrWindow(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1){
  // CASET
  csLow(); dcCmd(); wr8(0x2A); csHigh();
  csLow(); dcData(); wr8(x0>>8); wr8(x0); wr8(x1>>8); wr8(x1); csHigh();
  // RASET
  csLow(); dcCmd(); wr8(0x2B); csHigh();
  csLow(); dcData(); wr8(y0>>8); wr8(y0); wr8(y1>>8); wr8(y1); csHigh();
  // RAMWR
  csLow(); dcCmd(); wr8(0x2C); dcData(); // leave selected for streaming
}

// 24-bit flood (R,G,B) — matches COLMOD=0x66 (18-bit)
static void fillRect24(int16_t x,int16_t y,int16_t w,int16_t h, uint8_t r,uint8_t g,uint8_t b){
  if (w<=0 || h<=0) return;
  setAddrWindow(x,y,x+w-1,y+h-1);
  uint32_t n = (uint32_t)w * (uint32_t)h;
  while(n--){ wr8(r); wr8(g); wr8(b); }
  csHigh();
}

static void drawFrame1px(uint8_t r,uint8_t g,uint8_t b){
  fillRect24(0, 0, g_w, 1, r,g,b);
  fillRect24(0, g_h-1, g_w, 1, r,g,b);
  fillRect24(0, 0, 1, g_h, r,g,b);
  fillRect24(g_w-1, 0, 1, g_h, r,g,b);
}

static void hardReset(){
  pinMode(PIN_TFT_RST, OUTPUT);
  digitalWrite(PIN_TFT_RST, LOW);  delay(60);
  digitalWrite(PIN_TFT_RST, HIGH); delay(160);
}

// “VIVID” power/gamma (removes pastel/washed look)
static const uint8_t SEQ_VIVID[] = {
  0xC0,2,0x10,0x10,     // Power ctrl 1
  0xC1,1,0x41,          // Power ctrl 2
  0xC5,3,0x00,0x2C,0x80,// VCOM
  0xB0,1,0x00,          // Interface mode
  0xB1,1,0xB0,          // Frame rate
  0xB4,1,0x02,          // Inversion ctrl (2-dot)
  0xB6,3,0x02,0x02,0x3B,// Display function
  0xE0,15,              // Positive gamma
    0x00,0x03,0x09,0x08,0x16,0x0A,0x3F,0x78,0x4C,0x09,0x0A,0x08,0x16,0x1A,0x0F,
  0xE1,15,              // Negative gamma
    0x00,0x16,0x19,0x03,0x0F,0x05,0x32,0x45,0x46,0x04,0x0E,0x0D,0x35,0x37,0x0F,
  0xE9,1,0x00,
  0xF7,4,0xA9,0x51,0x2C,0x82
};

static void ili9488_init(){
  // SWRESET → SLPOUT
  cmd(0x01); delay(150);
  cmd(0x11); delay(120);

  // Power/gamma (vivid)
  writeSeq(SEQ_VIVID, sizeof(SEQ_VIVID));

  // Pixel format = 18-bit
  cmd(0x3A); data1(0x66);

  // Inversion ON (key for this panel)
  cmd(0x21);

  // Display ON
  cmd(0x29); delay(30);
}

// MADCTL values for rotations 0..3 (all include BGR)
static const uint8_t MAD_BY_ROT[4] = { 0x48, 0x28, 0x88, 0xE8 };

static void setRotation(uint8_t rot){
  rot &= 3;
  if (rot & 1) { g_w = 480; g_h = 320; } else { g_w = 320; g_h = 480; }
  cmd(0x36); data1(MAD_BY_ROT[rot]); // MADCTL
}

static void demoBars(){
  fillRect24(0,            0,    g_w, g_h/3,           255,0,0);
  fillRect24(0,            g_h/3,g_w, g_h/3,           0,255,0);
  fillRect24(0, 2*(g_h/3),       g_w, g_h-2*(g_h/3),   0,0,255);
  drawFrame1px(255,255,0);
}

void setup(){
  // Pins & SPI
  pinMode(PIN_TFT_CS, OUTPUT);
  pinMode(PIN_TFT_DC, OUTPUT);
  csHigh(); digitalWrite(PIN_TFT_DC, HIGH);

  SPI.begin();
  SPI.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));

  hardReset();
  ili9488_init();

  // Choose orientation: 0 (MADCTL 0x48) or 2 (MADCTL 0x88) were confirmed good.
  setRotation(0);  // ← change to 2 if you prefer the other good orientation

  demoBars();
}

void loop(){
  // Heartbeat square (magenta) so you see it's alive
  static bool on=false; on=!on;
  int y = (g_h > 320) ? 100 : 60;
  fillRect24(8, y, 34, 34, on?255:0, 0, on?255:0);
  delay(350);
}
