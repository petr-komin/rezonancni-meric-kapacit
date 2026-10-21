#include <math.h>
#include "kapacita.h"

// A = 1/f^2 = a*Cx + b,  Cx v pF, f v Hz
static double a = 0.0;
static double b = 0.0;

static inline double zFrekvence(double fHz) { return 1.0 / (fHz * fHz); }

bool kapacitaKalibruj(double f1Hz, double c1Pf, double f2Hz, double c2Pf) {
  if (f1Hz <= 0.0 || f2Hz <= 0.0) return false;
  if (c1Pf < 0.0 || c2Pf < 0.0) return false;
  if (c1Pf == c2Pf) return false;

  const double A1 = zFrekvence(f1Hz);
  const double A2 = zFrekvence(f2Hz);

  const double smernice = (A1 - A2) / (c1Pf - c2Pf);
  // Vetsi kapacita musi dat nizsi frekvenci, tedy vetsi A.
  // Zaporna smernice znamena prohozena mereni nebo chybu.
  if (smernice <= 0.0) return false;

  a = smernice;
  b = A1 - a * c1Pf;
  if (b <= 0.0) return false;  // C0 vyslo zaporne, nesmysl

  return true;
}

double kapacitaPf(double fHz) {
  if (a <= 0.0 || fHz <= 0.0) return -1.0;
  return (zFrekvence(fHz) - b) / a;
}

bool kapacitaNuluj(double f0Hz) {
  if (a <= 0.0 || f0Hz <= 0.0) return false;
  const double noveB = zFrekvence(f0Hz);
  if (noveB <= 0.0) return false;
  b = noveB;
  return true;
}

double kapacitaDejC0() { return (a > 0.0) ? (b / a) : 0.0; }

double kapacitaDejL() {
  if (a <= 0.0) return 0.0;
  // a = 4*pi^2*L*1e-12  (kapacita v pF)  ->  L v henry
  const double L = a / (4.0 * M_PI * M_PI * 1e-12);
  return L * 1e6;  // v uH
}

double kapacitaDejF0() {
  if (b <= 0.0) return 0.0;
  return 1.0 / sqrt(b);  // Cx = 0  ->  A = b
}

bool kapacitaJeKalibrovana() { return a > 0.0 && b > 0.0; }

void kapacitaNastavKonstanty(double novaA, double novaB) {
  a = novaA;
  b = novaB;
}

double kapacitaDejA() { return a; }
double kapacitaDejB() { return b; }
