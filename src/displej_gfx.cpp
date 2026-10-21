#include <Arduino.h>
#include "pins.h"
#include "displej_gfx.h"

// Piny sahame primo pres registry, digitalWrite je na kresleni moc
// pomaly. BSRR: spodnich 16 bitu nastavuje, hornich 16 shazuje.
#define SCK_H() (GPIOA->BSRR = (1u << 5))
#define SCK_L() (GPIOA->BSRR = (1u << (5 + 16)))
#define SDA_H() (GPIOA->BSRR = (1u << 7))
#define SDA_L() (GPIOA->BSRR = (1u << (7 + 16)))
#define CS_H() (GPIOA->BSRR = (1u << 4))
#define CS_L() (GPIOA->BSRR = (1u << (4 + 16)))
#define DC_H() (GPIOB->BSRR = (1u << 1))
#define DC_L() (GPIOB->BSRR = (1u << (1 + 16)))
#define RST_H() (GPIOB->BSRR = (1u << 0))
#define RST_L() (GPIOB->BSRR = (1u << (0 + 16)))

// MADCTL
#define MAD_MY 0x80
#define MAD_MX 0x40
#define MAD_MV 0x20
#define MAD_BGR 0x08

// Syrovy bitovy proud z posledniho cteni ID - na ladeni zarovnani.
// Cte se pres ST-Link, viz README.
volatile uint32_t g_idSyrovyA = 0;  // bity 0..31

Displej::Displej(int16_t sirka, int16_t vyska, uint8_t pX, uint8_t pY)
    : Adafruit_GFX(sirka, vyska),
      zakladSirka(sirka),
      zakladVyska(vyska),
      zakladX(pX),
      zakladY(pY),
      posunX(pX),
      posunY(pY),
      zpozdeni(8) {}

// Zdrzeni mezi hranami. Ridici promenna NENI volatile - to by
// prekladac musel pri kazdem pruchodu ukladat a nacitat z pameti
// a rezie by nekolikanasobne prevysila samotne klepani. Smycku
// pred optimalizaci chrani __NOP(), ktery je sam o sobe volatile.
// Poznamka pro budoucnost: pokus zrychlit tuhle rutinu slucovanim
// zapisu SDA a SCK do jednoho pristupu do BSRR a predpocitanim
// bitoveho vzoru skoncil tim, ze prenos ztracel synchronizaci
// uprostred vyplne - spodni cast plochy se zaplnila sumem. Rychlost
// je tu nepodstatna, prekresluji se male oblasti. Nechat tak.

void Displej::posliBajt(uint8_t b) {
  for (uint8_t i = 0; i < 8; i++) {
    if (b & 0x80) {
      SDA_H();
    } else {
      SDA_L();
    }
    b <<= 1;
    SCK_H();
    for (volatile uint16_t z = 0; z < zpozdeni; z++) {
    }
    SCK_L();  // radic vzorkuje na nabezne hrane
    for (volatile uint16_t z = 0; z < zpozdeni; z++) {
    }
  }
}

void Displej::prikaz(uint8_t c, const uint8_t *data, uint8_t n) {
  CS_L();
  DC_L();
  posliBajt(c);
  if (n) {
    DC_H();
    for (uint8_t i = 0; i < n; i++) posliBajt(data[i]);
  }
  CS_H();
}

void Displej::hwReset() {
  RST_H();
  delay(10);
  RST_L();
  delay(20);
  RST_H();
  delay(150);
}

void Displej::zacni() {
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(TFT_SCLK, OUTPUT);
  pinMode(TFT_MOSI, OUTPUT);
  SCK_L();
  CS_H();

  // inicializace jde schvalne pomalu, jde o ni jen jednou
  uint16_t puvodni = zpozdeni;
  zpozdeni = 40;

  hwReset();

  prikaz(0x01);  // SWRESET
  delay(150);
  prikaz(0x11);  // SLPOUT
  delay(500);

  const uint8_t frmctr[] = {0x01, 0x2C, 0x2D};
  prikaz(0xB1, frmctr, 3);  // FRMCTR1
  prikaz(0xB2, frmctr, 3);  // FRMCTR2
  const uint8_t frmctr3[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
  prikaz(0xB3, frmctr3, 6);  // FRMCTR3

  const uint8_t invctr[] = {0x07};
  prikaz(0xB4, invctr, 1);

  // budici napeti panelu - bez nich zustane displej tmavy
  const uint8_t pw1[] = {0xA2, 0x02, 0x84};
  prikaz(0xC0, pw1, 3);
  const uint8_t pw2[] = {0xC5};
  prikaz(0xC1, pw2, 1);
  const uint8_t pw3[] = {0x0A, 0x00};
  prikaz(0xC2, pw3, 2);
  const uint8_t pw4[] = {0x8A, 0x2A};
  prikaz(0xC3, pw4, 2);
  const uint8_t pw5[] = {0x8A, 0xEE};
  prikaz(0xC4, pw5, 2);
  const uint8_t vmctr[] = {0x0E};
  prikaz(0xC5, vmctr, 1);  // VCOM

  prikaz(0x20);  // INVOFF

  const uint8_t colmod[] = {0x05};  // 16 bitu na pixel
  prikaz(0x3A, colmod, 1);

  const uint8_t gamap[] = {0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
                           0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10};
  prikaz(0xE0, gamap, 16);
  const uint8_t gaman[] = {0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
                           0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10};
  prikaz(0xE1, gaman, 16);

  prikaz(0x13);  // NORON
  delay(10);
  prikaz(0x29);  // DISPON
  delay(100);

  setRotation(0);
  zpozdeni = puvodni;
}

void Displej::nastavGeometrii(int16_t sirka, int16_t vyska, uint8_t posX,
                              uint8_t posY) {
  zakladSirka = sirka;
  zakladVyska = vyska;
  zakladX = posX;
  zakladY = posY;
  setRotation(rotation);  // prepocita _width/_height i posun
}

void Displej::setRotation(uint8_t r) {
  rotation = r & 3;
  uint8_t madctl;
  switch (rotation) {
    default:
    case 0:
      madctl = MAD_MX | MAD_MY | MAD_BGR;
      _width = zakladSirka;
      _height = zakladVyska;
      posunX = zakladX;
      posunY = zakladY;
      break;
    case 1:
      madctl = MAD_MY | MAD_MV | MAD_BGR;
      _width = zakladVyska;
      _height = zakladSirka;
      posunX = zakladY;
      posunY = zakladX;
      break;
    case 2:
      madctl = MAD_BGR;
      _width = zakladSirka;
      _height = zakladVyska;
      posunX = zakladX;
      posunY = zakladY;
      break;
    case 3:
      madctl = MAD_MX | MAD_MV | MAD_BGR;
      _width = zakladVyska;
      _height = zakladSirka;
      posunX = zakladY;
      posunY = zakladX;
      break;
  }
  prikaz(0x36, &madctl, 1);
}

void Displej::okno(int16_t x, int16_t y, int16_t w, int16_t h) {
  uint8_t x0 = x + posunX, x1 = x + w - 1 + posunX;
  uint8_t y0 = y + posunY, y1 = y + h - 1 + posunY;
  const uint8_t caset[] = {0x00, x0, 0x00, x1};
  prikaz(0x2A, caset, 4);
  const uint8_t raset[] = {0x00, y0, 0x00, y1};
  prikaz(0x2B, raset, 4);
}

void Displej::naplnBarvou(uint16_t barva, uint32_t pocet) {
  const uint8_t hi = barva >> 8, lo = barva & 0xFF;
  CS_L();
  DC_L();
  posliBajt(0x2C);  // RAMWR
  DC_H();
  while (pocet--) {
    posliBajt(hi);
    posliBajt(lo);
  }
  CS_H();
}

void Displej::vyplnCelouRam(uint16_t barva) {
  // Cela pamet radice, bez ohledu na rozmery panelu a posun
  // pocatku. Na kalibraci: po tomhle uz nikde nemuze zustat
  // stary obsah. Pamet si ho jinak drzi, dokud nejde dolu napajeni.
  const uint8_t caset[] = {0x00, 0, 0x00, 131};
  prikaz(0x2A, caset, 4);
  const uint8_t raset[] = {0x00, 0, 0x00, 161};
  prikaz(0x2B, raset, 4);
  naplnBarvou(barva, 132UL * 162UL);
}

void Displej::drawPixel(int16_t x, int16_t y, uint16_t barva) {
  if (x < 0 || y < 0 || x >= _width || y >= _height) return;
  okno(x, y, 1, 1);
  naplnBarvou(barva, 1);
}

void Displej::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t barva) {
  if (w <= 0 || h <= 0 || x >= _width || y >= _height) return;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > _width) w = _width - x;
  if (y + h > _height) h = _height - y;
  if (w <= 0 || h <= 0) return;

  okno(x, y, w, h);
  naplnBarvou(barva, (uint32_t)w * h);
}

void Displej::fillScreen(uint16_t barva) {
  fillRect(0, 0, _width, _height, barva);
}

void Displej::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t barva) {
  fillRect(x, y, 1, h, barva);
}

void Displej::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t barva) {
  fillRect(x, y, w, 1, barva);
}

uint32_t Displej::ctiId() {
  const uint16_t puvodni = zpozdeni;
  zpozdeni = 40;  // cteni schvalne pomalu

  CS_L();
  DC_L();
  posliBajt(0x04);  // RDDID
  DC_H();

  pinMode(TFT_MOSI, INPUT);
  uint32_t syrovy = 0;
  for (uint8_t i = 0; i < 32; i++) {
    syrovy = (syrovy << 1) | (digitalRead(TFT_MOSI) ? 1 : 0);
    SCK_H();
    for (volatile uint16_t z = 0; z < zpozdeni; z++) {
    }
    SCK_L();
    for (volatile uint16_t z = 0; z < zpozdeni; z++) {
    }
  }
  pinMode(TFT_MOSI, OUTPUT);
  CS_H();
  zpozdeni = puvodni;

  g_idSyrovyA = syrovy;
  // Prvni bit je jalovy, pak 24 datovych.
  return (syrovy >> 7) & 0xFFFFFF;
}


