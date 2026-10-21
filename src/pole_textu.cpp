#include <string.h>
#include <math.h>
#include "pole_textu.h"

void PoleTextu::nastav(Adafruit_GFX *displej, int16_t novyX, int16_t novyY,
                       uint8_t novaVelikost, uint16_t novePopredi,
                       uint16_t novePozadi) {
  d = displej;
  x = novyX;
  y = novyY;
  velikost = novaVelikost;
  popredi = novePopredi;
  pozadi = novePozadi;
  zapomen();
}

void PoleTextu::zapomen() {
  for (uint8_t i = 0; i <= POLE_MAX_ZNAKU; i++) minuly[i] = 0;
}

void PoleTextu::vypis(const char *text) {
  if (!d) return;

  const int16_t sirkaZnaku = 6 * velikost;
  const uint8_t delka = strlen(text);

  for (uint8_t i = 0; i < POLE_MAX_ZNAKU; i++) {
    // Kratsi text se doplnuje mezerami, aby se smazal zbytek
    // predchoziho. Mezera se kresli i s pozadim, takze bunku vycisti.
    const char novy = (i < delka) ? text[i] : ' ';
    const char stary = minuly[i] ? minuly[i] : ' ';

    if (novy == stary) continue;

    d->drawChar(x + i * sirkaZnaku, y, novy, popredi, pozadi, velikost);
    minuly[i] = novy;
  }
}

void naRetezec(char *buf, uint8_t velikostBuf, double hodnota,
               uint8_t desetin) {
  if (velikostBuf < 2) {
    if (velikostBuf) buf[0] = 0;
    return;
  }

  uint8_t p = 0;
  if (hodnota < 0.0) {
    buf[p++] = '-';
    hodnota = -hodnota;
  }

  uint32_t nasobek = 1;
  for (uint8_t i = 0; i < desetin; i++) nasobek *= 10;

  // Zaokrouhleni na pozadovany pocet desetinnych mist
  const uint32_t cele = (uint32_t)(hodnota * nasobek + 0.5);
  uint32_t predTeckou = cele / nasobek;
  uint32_t zaTeckou = cele % nasobek;

  // celou cast pozpatku
  char pom[12];
  uint8_t n = 0;
  if (predTeckou == 0) pom[n++] = '0';
  while (predTeckou && n < sizeof(pom)) {
    pom[n++] = '0' + (predTeckou % 10);
    predTeckou /= 10;
  }
  while (n && p < velikostBuf - 1) buf[p++] = pom[--n];

  if (desetin && p < velikostBuf - 1) {
    buf[p++] = '.';
    for (uint8_t i = desetin; i > 0 && p < velikostBuf - 1; i--) {
      uint32_t del = 1;
      for (uint8_t k = 1; k < i; k++) del *= 10;
      buf[p++] = '0' + ((zaTeckou / del) % 10);
    }
  }
  buf[p] = 0;
}

void naRetezecSkupiny(char *buf, uint8_t velikostBuf, uint32_t hodnota) {
  char pom[12];
  uint8_t n = 0;
  if (hodnota == 0) pom[n++] = '0';
  while (hodnota && n < sizeof(pom)) {
    pom[n++] = '0' + (hodnota % 10);
    hodnota /= 10;
  }

  uint8_t p = 0;
  for (int8_t i = n - 1; i >= 0 && p < velikostBuf - 1; i--) {
    buf[p++] = pom[i];
    if (i && (i % 3 == 0) && p < velikostBuf - 1) buf[p++] = ' ';
  }
  buf[p] = 0;
}
