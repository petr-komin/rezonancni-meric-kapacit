#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------
// Prepocet rezonancni frekvence na kapacitu.
//
// Merena kapacita se pripojuje PARALELNE, takze se scita s tim,
// co uz v obvodu je:
//
//     f = 1 / (2*pi*sqrt(L * (C0 + Cx)))
//
// Po umocneni a prevraceni je vztah LINEARNI:
//
//     1/f^2 = 4*pi^2*L * (C0 + Cx)
//
// Kdyz oznacime A = 1/f^2, je A = a*Cx + b, tedy primka. Staci
// tedy dve mereni se ZNAMYMI kondenzatory a mame obe konstanty:
//
//     a = (A1 - A2) / (C1 - C2)
//     b = A1 - a*C1
//
// A z nich pak cokoliv:
//     Cx = (A - b) / a       merena kapacita
//     C0 = b / a             kapacita obvodu naprazdno
//     L  = a / (4*pi^2*1e-12)  indukcnost civky
//
// Vyhoda proti puvodnimu postupu: NENI potreba merit naprazdno.
// Na vysokych frekvencich tam byva prilis mala amplituda, nez aby
// citac neco napocital.
//
// Presnost stoji a pada na presnosti tech dvou kondenzatoru.
// Vyplati se poridit slidove s tolerenci 1 %.
// ---------------------------------------------------------------

// Kalibrace dvema merenimi se znamymi kondenzatory [Hz, pF].
// Vraci true pri uspechu. Kondenzatory se musi vyrazne lisit,
// jinak je vysledek necitlivy na chyby mereni.
bool kapacitaKalibruj(double f1Hz, double c1Pf, double f2Hz, double c2Pf);

// Merena kapacita v pF. Zaporna hodnota znamena, ze jeste neni
// kalibrovano.
double kapacitaPf(double fHz);

// Dopocitane hodnoty obvodu - na kontrolu, jestli kalibrace dava smysl.
double kapacitaDejC0();  // kapacita obvodu naprazdno [pF]
double kapacitaDejL();   // indukcnost civky [uH]
double kapacitaDejF0();  // vypoctena frekvence naprazdno [Hz]

bool kapacitaJeKalibrovana();

// Vynulovani: prepocita offset b z aktualni frekvence naprazdno.
// Sklon a zustava - ten zavisi jen na indukcnosti civky, ktera se
// pripravkem nemeni. Meni se jen C0, tedy parazitni kapacita
// pripravku a obvodu.
//
// Nulovat se ma az po tepelnem ustaleni, proto je to na tlacitku
// a ne automaticky pri zapnuti.
bool kapacitaNuluj(double f0Hz);

// Rucni nastaveni konstant, kdyz uz je kalibrace znama z drivejska.
void kapacitaNastavKonstanty(double a, double b);
double kapacitaDejA();
double kapacitaDejB();
