#pragma once
#include <Adafruit_GFX.h>

// ---------------------------------------------------------------
// Textove pole, ktere se prekresluje bez blikani.
//
// Pamatuje si, co na nem naposledy bylo, a pri zmene sahne jen na
// znaky, ktere se opravdu lisi. Pri pocitani sekund se tak typicky
// hybe jen posledni cislice.
//
// Kazdy znak se kresli VCETNE POZADI jednim pruchodem, takze se
// nikdy neobjevi cerna mezera mezi smazanim a vypsanim - presne to
// zpusobuje blikani.
// ---------------------------------------------------------------

#define POLE_MAX_ZNAKU 24

class PoleTextu {
 public:
  PoleTextu() {}

  // x, y = levy horni roh, velikost = nasobek zakladniho pisma 6x8
  void nastav(Adafruit_GFX *displej, int16_t x, int16_t y, uint8_t velikost,
              uint16_t popredi, uint16_t pozadi);

  // Zmena barvy textu bez prekresleni; projevi se pri dalsim vypisu.
  void nastavPopredi(uint16_t barva) { popredi = barva; }

  // Vypise text. Prekresli jen zmenene znaky.
  void vypis(const char *text);

  // Zapomene predchozi obsah - vola se po smazani cele plochy,
  // jinak by se pole domnivalo, ze uz je vykreslene.
  void zapomen();

  int16_t sirkaPixelu() const { return POLE_MAX_ZNAKU * 6 * velikost; }
  int16_t vyskaPixelu() const { return 8 * velikost; }

 private:
  Adafruit_GFX *d = nullptr;
  int16_t x = 0, y = 0;
  uint8_t velikost = 1;
  uint16_t popredi = 0xFFFF, pozadi = 0x0000;
  char minuly[POLE_MAX_ZNAKU + 1] = {0};
};

// Prevod desetinneho cisla na retezec bez pouziti %f.
// Knihovna printf v uspornem provedeni newlib desetinna cisla
// neumi a zapinat je kvuli dvema hodnotam se nevyplati.
void naRetezec(char *buf, uint8_t velikostBuf, double hodnota,
               uint8_t desetin);

// Cele cislo po skupinach po trech: 36 000 000
void naRetezecSkupiny(char *buf, uint8_t velikostBuf, uint32_t hodnota);
