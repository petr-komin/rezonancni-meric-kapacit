#include <Arduino.h>
#include "frekvence.h"

// ---------------------------------------------------------------
// Casovace si obsluhujeme primo pres registry, bez HardwareTimer.
//
// Duvod: s HardwareTimer se hradlovaci preruseni po dvou tiknutich
// prestalo volat, i kdyz casovac prokazatelne bezel dal (CR1 mel
// CEN, CNT postupoval, UIF se nastavoval). Preruseni se proste
// nedostalo k nasi funkci. Takhle mame plnou kontrolu.
//
// TIM2 = citac pulzu, hodinovym zdrojem je primo pin ETR (PA0).
//        Procesor pulzy nepocita, jen obcas obslouzi preteceni
//        16bitoveho citace a tim z nej udela 32bitovy.
// TIM3 = hradlovaci perioda. Na jejim konci se citac odecte.
//
// Obsluhy si musime protlacit pres tabulku vektoru presunutou do
// RAM. Jinak to nejde: HardwareTimer.cpp z jadra je vzdy
// prilinkovana a TIM2_IRQHandler i TIM3_IRQHandler si definuje sama,
// takze by doslo ke kolizi symbolu. A nechat je na jadru taky nelze -
// kdyz v obsluze nenajde svuj objekt HardwareTimer, NESMAZE priznak
// preruseni a procesor se zahlti nekonecnou smyckou.
// ---------------------------------------------------------------

// STM32F103: 16 systemovych vektoru + 60 preruseni.
// VTOR vyzaduje zarovnani na mocninu dvojky vetsi nez tabulka.
#define POCET_VEKTORU 76
static uint32_t vektoryRam[POCET_VEKTORU] __attribute__((aligned(512)));

// Diagnostika, da se precist pres ST-Link:
//   arm-none-eabi-nm firmware.elf | grep g_isr
volatile uint32_t g_isrPreteceni = 0;
volatile uint32_t g_isrBrana = 0;

static volatile uint32_t preteceni = 0;  // horni 16 bitu citace
static volatile uint32_t pulzy = 0;      // za posledni branu
static volatile bool novaHodnota = false;
static volatile uint32_t predchoziStav = 0;
static uint16_t branaDelkaMs = 1000;

// Slozi 32bitovy stav citace. Cte se opakovane, dokud se horni a
// spodni cast nesejdou - jinak by odecet presne v okamziku
// preteceni dal nesmyslnou hodnotu.
static uint32_t ctiStav() {
  uint32_t horni1, spodni, horni2;
  for (uint8_t pokus = 0; pokus < 4; pokus++) {
    horni1 = preteceni;
    spodni = TIM2->CNT;
    horni2 = preteceni;
    if (horni1 == horni2) return (horni1 << 16) | spodni;
  }
  return (preteceni << 16) | TIM2->CNT;
}

// Preteceni citace pulzu.
static void obsluhaTim2(void) {
  if (TIM2->SR & TIM_SR_UIF) {
    TIM2->SR = ~TIM_SR_UIF;
    preteceni++;
    g_isrPreteceni++;
  }
}

// Konec hradlovaci periody.
static void obsluhaTim3(void) {
  if (TIM3->SR & TIM_SR_UIF) {
    TIM3->SR = ~TIM_SR_UIF;
    g_isrBrana++;

    uint32_t stav = ctiStav();
    // Rozdil proti minulemu odectu. Citac se nenuluje, takze se
    // neztrati pulz na prelomu period; preteceni 32 bitu se
    // v odectu vyrusi samo. Nasobenim delickou se dostaneme zpet
    // ke skutecnemu poctu pulzu na vstupu.
    pulzy = (stav - predchoziStav) * ETR_DELICKA;
    predchoziStav = stav;
    novaHodnota = true;
  }
}

// Presune tabulku vektoru do RAM a prepise v ni dve polozky.
static void presunVektory() {
  const uint32_t *zdroj = (const uint32_t *)SCB->VTOR;
  for (uint16_t i = 0; i < POCET_VEKTORU; i++) vektoryRam[i] = zdroj[i];

  vektoryRam[16 + TIM2_IRQn] = (uint32_t)obsluhaTim2;
  vektoryRam[16 + TIM3_IRQn] = (uint32_t)obsluhaTim3;

  __disable_irq();
  SCB->VTOR = (uint32_t)vektoryRam;
  __DSB();
  __ISB();
  __enable_irq();
}

void frekvenceInit(uint16_t branaMs) {
  presunVektory();

  // Stazeni dolu, aby vstup bez pripojeneho signalu necital sum.
  // Zdroj signalu ho snadno prepere, je to jen slaby odpor.
  pinMode(PIN_FREKVENCE, INPUT_PULLDOWN);

  // --- TIM2: citac pulzu, hodiny z pinu ETR ---
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  TIM2->CR1 = 0;
  TIM2->PSC = 0;
  TIM2->ARR = 0xFFFF;
  // ECE  = externi hodiny rezim 2, zdrojem je pin ETR
  // ETPS = delicka pred vzorkovanim (bity 13:12)
  // ETF  = filtr (bity 11:8); pri vysokych frekvencich musi byt
  //        slabsi, filtr na 8 vzorku srazi strop asi na 4,5 MHz
  // ETP  = 0, citame nabezne hrany
  uint32_t etps = 0;
#if ETR_DELICKA == 2
  etps = 1;
#elif ETR_DELICKA == 4
  etps = 2;
#elif ETR_DELICKA == 8
  etps = 3;
#endif
  // Poradi v hardwaru je: pin -> polarita -> DELICKA -> filtr.
  // Filtr tedy uz vidi podelenou frekvenci a muze byt mirny.
  const uint32_t etf = (ETR_DELICKA > 1) ? 0x1UL : 0x0UL;
  TIM2->SMCR = TIM_SMCR_ECE | (etps << 12) | (etf << 8);
  TIM2->EGR = TIM_EGR_UG;  // prepis registru do stinovych
  TIM2->SR = 0;
  TIM2->CNT = 0;
  TIM2->DIER = TIM_DIER_UIE;
  TIM2->CR1 = TIM_CR1_CEN;

  preteceni = 0;
  predchoziStav = 0;
  pulzy = 0;
  novaHodnota = false;
  g_isrPreteceni = 0;
  g_isrBrana = 0;

  // Citac ma prednost pred branou - preteceni se nesmi zmeskat.
  NVIC_SetPriority(TIM2_IRQn, 1);
  NVIC_EnableIRQ(TIM2_IRQn);

  // --- TIM3: hradlovaci perioda ---
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  TIM3->CR1 = 0;
  TIM3->PSC = 7199;  // 72 MHz / 7200 = 10 kHz, tedy krok 0,1 ms
  TIM3->DIER = TIM_DIER_UIE;
  NVIC_SetPriority(TIM3_IRQn, 2);
  NVIC_EnableIRQ(TIM3_IRQn);

  frekvenceNastavBranu(branaMs);
  TIM3->CR1 = TIM_CR1_CEN;
}

void frekvenceNastavBranu(uint16_t branaMs) {
  if (branaMs == 0) branaMs = 1;
  if (branaMs > 6553) branaMs = 6553;  // ARR je 16bitovy pri kroku 0,1 ms
  branaDelkaMs = branaMs;

  TIM3->ARR = (uint32_t)branaMs * 10 - 1;
  TIM3->EGR = TIM_EGR_UG;
  TIM3->SR = ~TIM_SR_UIF;  // UG nastavi UIF, nechceme hned preruseni
}

bool frekvenceNovaHodnota() {
  if (!novaHodnota) return false;
  novaHodnota = false;
  return true;
}

uint32_t frekvencePulzy() {
  NVIC_DisableIRQ(TIM3_IRQn);
  uint32_t p = pulzy;
  NVIC_EnableIRQ(TIM3_IRQn);
  return p;
}

double frekvenceHz() {
  return (double)frekvencePulzy() * 1000.0 / (double)branaDelkaMs;
}

uint32_t frekvenceHzCele() {
  uint64_t p = frekvencePulzy();
  return (uint32_t)((p * 1000ULL) / branaDelkaMs);
}

uint16_t frekvenceBrana() { return branaDelkaMs; }

uint32_t frekvenceCitac() { return ctiStav(); }
