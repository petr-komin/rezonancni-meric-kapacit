#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------
// Mereni frekvence hardwarovym citanim pulzu.
//
// TIM2 bezi v rezimu externich hodin 2 (ECE) - jeho hodinovym
// zdrojem je primo signal na pinu ETR, tedy PA0. Procesor tedy
// pulzy nepocita v software, jen obcas obslouzi preteceni
// 16bitoveho citace, cimz z nej udela 32bitovy.
//
// TIM4 mezitim odmeruje hradlovaci periodu. Na jejim konci se
// citac odecte a z rozdilu se spocita frekvence.
//
// Citac se nenuluje, jen se bere rozdil proti minulemu odectu -
// tim se neztrati zadny pulz na prelomu period.
//
// Vstup: PA0 (TIM2_ETR). Musi byt volny - displej pouziva
// PA4, PA5, PA7, PB0 a PB1.
// ---------------------------------------------------------------

#define PIN_FREKVENCE PA0

// Delicka primo ve vstupu ETR (1, 2, 4 nebo 8).
//
// Citac vzorkuje vstup vnitrnimi hodinami 72 MHz, takze maximalni
// vstupni frekvence je jejich polovina - 36 MHz. Na te hrane uz
// pocitani neni spolehlive. Delicka je hardwarovy delic PRED
// vzorkovanim, presne na tohle urceny: pri /2 vidi citac 18 MHz
// a ma pohodlnou rezervu.
//
// Napocitane pulzy se zpatky nasobi, takze vysledek je ve skutecne
// frekvenci. Cenou je rozliseni: pri /2 vychazi po 2 Hz misto 1 Hz,
// coz je na 36 MHz 0,06 ppm - nepodstatne.
#ifndef ETR_DELICKA
#define ETR_DELICKA 2
#endif

// Spusti mereni. branaMs = delka hradlovaci periody v ms.
// Delsi brana = jemnejsi rozliseni, ale pomalejsi odezva.
// Pri brane 1000 ms je rozliseni 1 Hz.
void frekvenceInit(uint16_t branaMs = 1000);

// Zmeni delku hradlovaci periody za behu.
void frekvenceNastavBranu(uint16_t branaMs);

// true, kdyz je k dispozici novy vysledek. Prectenim se priznak maze.
bool frekvenceNovaHodnota();

// Posledni namerena frekvence v Hz.
//
// Vraci double, ne float: float ma 24bitovou mantisu, takze nad
// 16,7 milionu uz neudrzi jednotky Hz a na 36 MHz by zahodil
// presnost, kterou mereni ma.
double frekvenceHz();

// Totez jako cele cislo - pri brane 1000 ms je to primo pocet pulzu.
uint32_t frekvenceHzCele();

// Pocet pulzu za posledni hradlovaci periodu.
uint32_t frekvencePulzy();

// Aktualni delka hradlovaci periody v ms.
uint16_t frekvenceBrana();

// Celkovy stav citace od startu - na ladeni.
uint32_t frekvenceCitac();
