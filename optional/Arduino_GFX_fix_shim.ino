// Arduino_GFX ILI9488 — post-begin() fixup to match the proven panel settings
// Use if you want Arduino_GFX drawing but need 18-bit + INVON + VIVID after begin().
// Pins: CS=D3, DC=D0, RST=D1; SPI via ICSP.

#include <Arduino_GFX_Library.h>

#define TFT_CS  3
#define TFT_DC  0
#define TFT_RST 1

Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS);
Arduino_GFX *gfx     = new Arduino_ILI9488(bus, TFT_RST, 0 /*rotation*/, false /*IPS*/);

static const uint32_t SPI_HZ = 16000000;

static const uint8_t vivid[] = {
  0xC0,2,0x10,0x10,  0xC1,1,0x41,  0xC5,3,0x00,0x2C,0x80,
  0xB0,1,0x00,      0xB1,1,0xB0,  0xB4,1,0x02,  0xB6,3,0x02,0x02,0x3B,
  0xE0,15,0x00,0x03,0x09,0x08,0x16,0x0A,0x3F,0x78,0x4C,0x09,0x0A,0x08,0x16,0x1A,0x0F,
  0xE1,15,0x00,0x16,0x19,0x03,0x0F,0x05,0x32,0x45,0x46,0x04,0x0E,0x0D,0x35,0x37,0x0F,
  0xE9,1,0x00,      0xF7,4,0xA9,0x51,0x2C,0x82
};

static void post_begin_fixup(){
  gfx->startWrite();
  // Ensure 18-bit pixel format
  gfx->writeCommand(0x3A); gfx->write(0x66);
  // VIVID sequence
  const uint8_t *p=vivid; size_t i=0, n=sizeof(vivid);
  while(i<n){
    uint8_t reg=p[i++], cnt=p[i++];
    gfx->writeCommand(reg);
    while(cnt--) gfx->write(*p++);
  }
  // Inversion ON
  gfx->writeCommand(0x21);
  gfx->endWrite();
}

void setup(){
  if (!gfx->begin(SPI_HZ)) while(1){};
  post_begin_fixup();
  gfx->setRotation(0); // 0→MADCTL 0x48, 2→0x88
  gfx->fillScreen(BLACK);
  gfx->fillRect(0,0,gfx->width(),gfx->height()/3, RED);
  gfx->fillRect(0,gfx->height()/3,gfx->width(),gfx->height()/3, GREEN);
  gfx->fillRect(0,2*(gfx->height()/3),gfx->width(),gfx->height()-2*(gfx->height()/3), BLUE);
}

void loop(){
  static bool on=false; on=!on;
  gfx->fillRect(8, 100, 34, 34, on?MAGENTA:BLACK);
  delay(350);
}
