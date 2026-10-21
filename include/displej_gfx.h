#pragma once
#include <Adafruit_GFX.h>

// ---------------------------------------------------------------
// Vlastni ovladac ST7735 postaveny na Adafruit_GFX.
//
// Proc vlastni a ne Adafruit_ST7735:
//  * initR() vola commonInit() -> begin() bez argumentu, takze cela
//    inicializace jde vzdy na SPI_DEFAULT_FREQ = 32 MHz bez ohledu
//    na setSPISpeed(). Na bezne kabelazi ji radic neprijme.
//  * Softwarove SPI knihovny na tehle desce take nekreslilo.
//  * Bitove klepani vlastnim kodem oveřene funguje.
//
// Kresleni jde primo do registru GPIO (BSRR), takze je podstatne
// rychlejsi nez digitalWrite. Rychlost se da ladit pres
// nastavZpozdeni() - vyssi cislo = pomalejsi a odolnejsi prenos.
// ---------------------------------------------------------------
// Geometrie panelu - overeno merenim: modul 1.8" 128x160 s nulovym
// posunem pocatku. Bily prouzek na prvnim radku okna lezi presne na
// hornim okraji panelu, zeleny na poslednim presne na spodnim.
//
// Vyska 160 je zaroven bezpecna i pro mensi panel 128x128: prebytecne
// radky se zapisou mimo viditelnou oblast a nic nezkazi. Opacna chyba
// by ukrojila spodek obrazu.
#ifndef PANEL_W
#define PANEL_W 128
#endif
#ifndef PANEL_H
#define PANEL_H 160
#endif
#ifndef PANEL_X
#define PANEL_X 0
#endif
#ifndef PANEL_Y
#define PANEL_Y 0
#endif

// Zpozdeni mezi hranami hodin pri kresleni. Vyssi = pomalejsi, ale
// odolnejsi vuci delsim propojkam. Zjisti se rezimem kalibrace.
//
// Kalibrace prosla v celem rozsahu 10 az 0; 10 je ale ve stavajici
// implementaci uz neunosne pomale (pres 2,5 s na celou plochu).
// Dvojka je bezpecne uvnitr overeneho rozsahu a da kolem 700 ms.
// Rychlost tu stejne nehraje roli - prekresluji se male oblasti
// s cisly, ne cela plocha.
#ifndef ZPOZDENI_KRESLENI
#define ZPOZDENI_KRESLENI 2
#endif

// Barvy RGB565
#define BARVA_CERNA 0x0000
#define BARVA_BILA 0xFFFF
#define BARVA_CERVENA 0xF800
#define BARVA_ZELENA 0x07E0
#define BARVA_MODRA 0x001F
#define BARVA_ZLUTA 0xFFE0
#define BARVA_AZUROVA 0x07FF
#define BARVA_FIALOVA 0xF81F
#define BARVA_SEDA 0x8410

class Displej : public Adafruit_GFX {
 public:
  Displej(int16_t sirka = PANEL_W, int16_t vyska = PANEL_H,
          uint8_t posunX = PANEL_X, uint8_t posunY = PANEL_Y);

  // Uplna inicializace radice vcetne napajecich a gamma registru.
  // Bez PWCTR1..PWCTR5 a VMCTR1 zustane panel tmavy, i kdyz radic
  // prikazy prijima a meni svuj stav.
  void zacni();

  // Prozvoneni radice - vraci RDDID, ST7735 ma 0x7C89F0.
  //
  // POZOR: funguje spolehlive jen HNED PO hardwarovem resetu, na
  // jeste neinicializovanem radici. Po inicializaci tenhle modul na
  // cteni neodpovida - linka po prikazu jen visi nahore. Na ladeni
  // je to presto uzitecne: kdyz radic odpovi, zije a slysi nas,
  // a problem je pak v inicializaci, ne v zapojeni.
  //
  // Syrovy bitovy proud zustava v g_idSyrovyA / g_idSyrovyB.
  uint32_t ctiId();

  // Prepise CELOU pamet radice (132x162 = jeho maximum), bez ohledu
  // na rozmery panelu a posun pocatku. Na kalibraci: po tomhle uz
  // nikde nemuze zustat nepřepsany sum.
  void vyplnCelouRam(uint16_t barva);

  // Zmena rozmeru panelu a posunu pocatku za behu (na kalibraci).
  void nastavGeometrii(int16_t sirka, int16_t vyska, uint8_t posX,
                       uint8_t posY);

  // Zpozdeni mezi hranami hodin pri kresleni (pocet prazdnych cyklu).
  void nastavZpozdeni(uint16_t n) { zpozdeni = n; }
  uint16_t dejZpozdeni() const { return zpozdeni; }

  void drawPixel(int16_t x, int16_t y, uint16_t barva) override;
  void fillScreen(uint16_t barva) override;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                uint16_t barva) override;
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t barva) override;
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t barva) override;
  void setRotation(uint8_t r) override;

 private:
  void posliBajt(uint8_t b);
  void prikaz(uint8_t c, const uint8_t *data = nullptr, uint8_t n = 0);
  void okno(int16_t x, int16_t y, int16_t w, int16_t h);
  void naplnBarvou(uint16_t barva, uint32_t pocet);
  void hwReset();

  int16_t zakladSirka, zakladVyska;
  uint8_t zakladX, zakladY;  // posun pocatku panelu
  uint8_t posunX, posunY;    // aktualni podle rotace
  volatile uint16_t zpozdeni;
};
