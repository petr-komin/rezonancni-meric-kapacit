// ---------------------------------------------------------------
// Rezonancni meric kapacit - test displeje
// Blue Pill STM32F103C8 + 1.44" SPI 128x128 (ST7735S)
//
// Dva rezimy, prepinaji se v platformio.ini:
//   [env:bluepill_f103c8] - normalni test displeje
//   [env:kalibrace]       - hledani nejvyssi spolehlive rychlosti
// ---------------------------------------------------------------

#include <Arduino.h>
#include "pins.h"
#include "displej_gfx.h"
#if !defined(REZIM_KALIBRACE) && !defined(REZIM_PINY)
#include "frekvence.h"
#include "kapacita.h"
#include "pole_textu.h"
#include "testsignal.h"
#endif

// Citace v RAM, ctou se pres ST-Link i bez serioveho portu:
//   arm-none-eabi-nm firmware.elf | grep g_faze
//   openocd ... -c "mdw <adresa>"
volatile uint32_t g_faze = 0;
volatile uint32_t g_cyklu = 0;
volatile uint32_t g_msSnimek = 0;  // doba prekresleni cele plochy
volatile uint32_t g_pulzy = 0;     // pulzy za posledni branu
volatile uint32_t g_hz = 0;        // posledni frekvence v Hz

Displej displej;

static void led(bool sviti) {
  digitalWrite(LED_ONBOARD, sviti ? LOW : HIGH);  // PC13 sviti pri LOW
}

static void blik(uint8_t kolikrat, uint16_t doba = 100) {
  for (uint8_t i = 0; i < kolikrat; i++) {
    led(true);
    delay(doba);
    led(false);
    delay(doba);
  }
}

// Zmeri, jak dlouho trva prekresleni cele plochy.
static uint32_t zmerSnimek() {
  uint32_t t0 = millis();
  displej.fillScreen(BARVA_CERNA);
  return millis() - t0;
}

#ifdef REZIM_PINY
// ---------------------------------------------------------------
// Promereni jednotlivych vodicu k displeji.
//
// Kazdy pin se na 4 s vytahne na 3,3 V a na 4 s stahne na 0 V.
// Pred kazdym blikne LED tolikrat, kolikaty v poradi je.
// Merit se musi NA STRANE DISPLEJE, ne na Blue Pillu - jde o to
// overit i propojku, ne jen vyvod procesoru.
// ---------------------------------------------------------------
static const uint32_t pinyTest[] = {TFT_CS, TFT_DC, TFT_RST, TFT_SCLK,
                                    TFT_MOSI};
static const char *pinyNazvy[] = {"CS (PA4)", "A0 (PB1)", "RST (PB0)",
                                  "SCK (PA5)", "SDA (PA7)"};

void setup() {
  pinMode(LED_ONBOARD, OUTPUT);
  led(false);
  Serial1.begin(115200);
  delay(50);
  Serial1.println();
  Serial1.println("=== PROMERENI VODICU K DISPLEJI ===");
  for (uint8_t i = 0; i < 5; i++) pinMode(pinyTest[i], OUTPUT);
}

void loop() {
  for (uint8_t i = 0; i < 5; i++) {
    g_cyklu++;
    g_faze = i + 1;

    // vsechny dolu, at je jasne, ktery se prave meri
    for (uint8_t j = 0; j < 5; j++) digitalWrite(pinyTest[j], LOW);

    blik(i + 1, 250);
    delay(500);

    Serial1.print(pinyNazvy[i]);
    Serial1.println(" -> 3.3V");
    digitalWrite(pinyTest[i], HIGH);
    delay(4000);

    Serial1.print(pinyNazvy[i]);
    Serial1.println(" -> 0V");
    digitalWrite(pinyTest[i], LOW);
    delay(4000);
  }
}

#elif defined(REZIM_KALIBRACE)
// ---------------------------------------------------------------
// Jedna geometrie, jedna otazka. BEZ citace (viz build_src_filter).
//
// Strida se:
//   A) cela pamet radice CERVENE - co zustane cervene, tam se
//      nekresli
//   B) nase okno CERNE, s BILYM prouzkem na prvnim radku a
//      ZELENYM na poslednim
//
// Kdyz je geometrie spravne, bily prouzek lezi na uplnem hornim
// okraji panelu, zeleny na uplnem spodnim a cervena zmizi beze
// zbytku.
// ---------------------------------------------------------------

#ifndef ZPOZDENI_TEST
#define ZPOZDENI_TEST 10
#endif

void setup() {
  pinMode(LED_ONBOARD, OUTPUT);
  led(false);
  Serial1.begin(115200);
  delay(50);
  Serial1.println();
  Serial1.println("=== GEOMETRIE, bez citace ===");
  Serial1.print("nastaveno ");
  Serial1.print(PANEL_W);
  Serial1.print("x");
  Serial1.print(PANEL_H);
  Serial1.print(" posun ");
  Serial1.print(PANEL_X);
  Serial1.print(",");
  Serial1.println(PANEL_Y);

  blik(6, 100);

  displej.zacni();
  displej.nastavZpozdeni(ZPOZDENI_TEST);
  g_faze = 1;
}

void loop() {
  g_cyklu++;

  // A) cela pamet cervene
  led(true);
  displej.vyplnCelouRam(BARVA_CERVENA);
  g_faze = 10;
  delay(1500);

  // B) nase okno cerne se znackami na prvnim a poslednim radku
  led(false);
  displej.fillScreen(BARVA_CERNA);
  displej.fillRect(0, 0, displej.width(), 4, BARVA_BILA);
  displej.fillRect(0, displej.height() - 4, displej.width(), 4,
                   BARVA_ZELENA);
  displej.setTextColor(BARVA_ZLUTA);
  displej.setTextSize(3);
  displej.setCursor(10, displej.height() / 2 - 10);
  displej.print("OK");
  g_faze = 11;
  delay(6000);
}

#else
// ---------------------------------------------------------------
// Mereni frekvence - pulzy se pocitaji hardwarove na PA0
// ---------------------------------------------------------------

// Vykresli jen oblast s hodnotou, ne celou plochu. Prekresleni
// cele plochy trva pres 100 ms, tohle je zlomek.
// ---------------------------------------------------------------
// Textova pole. Kazde si pamatuje svuj obsah a pri zmene prekresli
// jen znaky, ktere se lisi - zadne mazani cernym obdelnikem, tedy
// zadne blikani.
// ---------------------------------------------------------------
static PoleTextu poleHz, poleMHz, polePulzy, poleKapacita, poleJednotka,
    poleZapati;

static void pripravPole() {
  poleHz.nastav(&displej, 4, 48, 2, BARVA_ZELENA, BARVA_CERNA);
  poleMHz.nastav(&displej, 4, 68, 1, BARVA_AZUROVA, BARVA_CERNA);
  polePulzy.nastav(&displej, 4, 84, 1, BARVA_SEDA, BARVA_CERNA);
  poleKapacita.nastav(&displej, 4, 112, 3, BARVA_ZLUTA, BARVA_CERNA);
  poleJednotka.nastav(&displej, 4, 136, 1, BARVA_ZLUTA, BARVA_CERNA);
  poleZapati.nastav(&displej, 4, 148, 1, BARVA_SEDA, BARVA_CERNA);
}

// ---------------------------------------------------------------
// Nulovaci tlacitko
//
// Stisk se jen poznamena; samotne vynulovani probehne az s dalsim
// dokoncenym merenim, aby se pouzila cela hradlovaci perioda
// a ne zastarala hodnota.
// ---------------------------------------------------------------
static bool cekaNaNulovani = false;
static uint32_t zpozdeneZapati = 0;  // kdy vratit zapati zpet
static bool minulyStav = true;  // klidovy stav je H, tlacitko spina na zem
static uint32_t posledniZmena = 0;

static void obsluhaTlacitka() {
  const bool stav = digitalRead(PIN_NULOVANI);
  const uint32_t ted = millis();

  if (stav != minulyStav) {
    if (ted - posledniZmena < 50) return;  // zakmity
    posledniZmena = ted;
    minulyStav = stav;
    if (!stav) cekaNaNulovani = true;  // sestupna hrana = stisk
  }
}

static void zobrazZapati(const char *hlaska = nullptr) {
  char buf[POLE_MAX_ZNAKU + 1];
  if (hlaska) {
    poleZapati.nastavPopredi(BARVA_ZELENA);
    poleZapati.vypis(hlaska);
    return;
  }

  poleZapati.nastavPopredi(BARVA_SEDA);
  if (!kapacitaJeKalibrovana()) {
    poleZapati.vypis("nekalibrovano");
    return;
  }

  char c0[10], l[10];
  naRetezec(c0, sizeof(c0), kapacitaDejC0(), 2);
  naRetezec(l, sizeof(l), kapacitaDejL(), 2);
  snprintf(buf, sizeof(buf), "C0 %spF  L %suH", c0, l);
  poleZapati.vypis(buf);
}

static void zobrazFrekvenci(uint32_t hz, uint32_t pulzy) {
  char buf[POLE_MAX_ZNAKU + 1];

  naRetezecSkupiny(buf, sizeof(buf), hz);
  poleHz.vypis(buf);

  if (hz >= 1000000UL) {
    char d[14];
    naRetezec(d, sizeof(d), (double)hz / 1e6, 4);
    snprintf(buf, sizeof(buf), "%s MHz", d);
  } else {
    char d[14];
    naRetezec(d, sizeof(d), (double)hz / 1e3, 3);
    snprintf(buf, sizeof(buf), "%s kHz", d);
  }
  poleMHz.vypis(buf);

  snprintf(buf, sizeof(buf), "%lu pulzu / %u ms", (unsigned long)pulzy,
           frekvenceBrana());
  polePulzy.vypis(buf);
}

static void zobrazKapacitu(double pf) {
  char buf[POLE_MAX_ZNAKU + 1];

  if (!kapacitaJeKalibrovana() || pf < 0.0) {
    poleKapacita.nastavPopredi(BARVA_SEDA);
    poleKapacita.vypis("---");
    poleJednotka.vypis("");
    return;
  }

  poleKapacita.nastavPopredi(BARVA_ZLUTA);
  if (pf < 1000.0) {
    naRetezec(buf, sizeof(buf), pf, (pf < 100.0) ? 2 : 1);
    poleKapacita.vypis(buf);
    poleJednotka.vypis("pF");
  } else {
    naRetezec(buf, sizeof(buf), pf / 1000.0, 3);
    poleKapacita.vypis(buf);
    poleJednotka.vypis("nF");
  }
}

void setup() {
  pinMode(LED_ONBOARD, OUTPUT);
  led(false);
  g_faze = 1;

  // Zivotni znameni jeste pred sahnutim na displej. Kdyz tohle
  // nevidis, nebezi kod a displej neres.
  blik(6, 100);
  g_faze = 2;

  Serial1.begin(115200);  // PA9 = TX, PA10 = RX
  delay(50);
  Serial1.println();
  Serial1.println("=== Mereni frekvence, vstup PA0 ===");

  led(true);
  displej.zacni();
  displej.nastavZpozdeni(ZPOZDENI_KRESLENI);
  led(false);
  g_faze = 3;

  g_msSnimek = zmerSnimek();
  Serial1.print("prekresleni cele plochy: ");
  Serial1.print(g_msSnimek);
  Serial1.println(" ms");

  // staticka cast obrazovky, kresli se jednou
  displej.fillScreen(BARVA_CERNA);
  displej.drawRect(0, 0, displej.width(), displej.height(), BARVA_SEDA);
  displej.setTextColor(BARVA_BILA);
  displej.setTextSize(1);
  displej.setCursor(6, 8);
  displej.print("Meric kapacit");
  displej.setTextColor(BARVA_SEDA);
  displej.setCursor(6, 20);
  displej.print("vstup PA0");
  displej.drawFastHLine(0, 38, displej.width(), BARVA_SEDA);
  displej.drawFastHLine(0, 104, displej.width(), BARVA_SEDA);
  displej.drawFastHLine(0, 144, displej.width(), BARVA_SEDA);
  displej.setTextColor(BARVA_SEDA);
  displej.setCursor(70, 20);
  displej.print(g_msSnimek);
  displej.print(" ms");

#if defined(KAL_F1) && defined(KAL_C1) && defined(KAL_F2) && defined(KAL_C2)
  // Kalibrace ze dvou mereni se znamymi kondenzatory, viz
  // platformio.ini. Bez tlacitek je to nejjednodussi cesta -
  // zmeris dve frekvence, zapisou se sem a plati natrvalo.
  if (kapacitaKalibruj(KAL_F1, KAL_C1, KAL_F2, KAL_C2)) {
    Serial1.print("kalibrace OK: C0=");
    Serial1.print(kapacitaDejC0(), 2);
    Serial1.print(" pF, L=");
    Serial1.print(kapacitaDejL(), 3);
    Serial1.print(" uH, f0=");
    Serial1.print(kapacitaDejF0() / 1e6, 3);
    Serial1.println(" MHz");
  } else {
    Serial1.println("kalibrace SELHALA - zkontroluj hodnoty");
  }
#endif

  pinMode(PIN_NULOVANI, INPUT_PULLUP);  // nulovaci tlacitko na zem

  frekvenceInit(1000);  // brana 1 s -> rozliseni 1 Hz

#ifdef TESTSIGNAL_HZ
  // Zkusebni signal na PB6. Propoj PB6 -> PA0 a mericka musi ukazat
  // presne tuhle hodnotu - overi to cely merici retezec bez externiho
  // pristroje. Zapina se build flagem, viz platformio.ini.
  uint32_t ocekavano = testSignalInit(TESTSIGNAL_HZ);
  displej.setTextColor(BARVA_SEDA);
  displej.setCursor(6, 118);
  displej.print("test PB6 -> PA0");
  displej.setCursor(6, 130);
  displej.print(ocekavano);
  displej.print(" Hz");
  Serial1.print("zkusebni signal PB6 = ");
  Serial1.print(ocekavano);
  Serial1.println(" Hz");
#else
  displej.setTextColor(BARVA_SEDA);
  displej.setCursor(6, 118);
  displej.print("vstup PA0, max 3V3");

  pripravPole();  // az po vykresleni statickeho pozadi
  zobrazZapati();
#endif

  g_faze = 4;
}

void loop() {
  obsluhaTlacitka();
  if (!frekvenceNovaHodnota()) return;

  g_cyklu++;
  uint32_t pulzy = frekvencePulzy();
  uint32_t hz = frekvenceHzCele();
  g_pulzy = pulzy;
  g_hz = hz;

  // Vynulovani az tady, s cerstvou hodnotou z cele hradlovaci periody
  if (cekaNaNulovani) {
    cekaNaNulovani = false;
    if (kapacitaNuluj((double)hz)) {
      zobrazZapati("VYNULOVANO");
      Serial1.print("vynulovano, nove C0 = ");
      Serial1.print(kapacitaDejC0(), 3);
      Serial1.println(" pF");
      blik(2, 120);
    } else {
      zobrazZapati("nulovani selhalo");
    }
    posledniZmena = millis();
    zpozdeneZapati = millis() + 2500;
  }

  if (zpozdeneZapati && millis() > zpozdeneZapati) {
    zpozdeneZapati = 0;
    zobrazZapati();
  }

  zobrazFrekvenci(hz, pulzy);
  zobrazKapacitu(kapacitaPf((double)hz));

  // kontrolka, ze mereni bezi
  displej.fillRect(displej.width() - 10, 4, 5, 5,
                   (g_cyklu & 1) ? BARVA_ZELENA : BARVA_CERNA);

  Serial1.print(pulzy);
  Serial1.print(" pulzu -> ");
  Serial1.print(hz);
  Serial1.println(" Hz");
}
#endif
