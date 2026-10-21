#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------
// Generator zkusebniho signalu na PB6 (TIM4_CH1), strida 50 %.
//
// Na overeni celeho mericiho retezce bez externiho oscilatoru:
// propoj PB6 drátkem na PA0 a mericka musi ukazat presne tu
// frekvenci, ktera se nastavila.
//
// PB6 je volny - displej pouziva PA4, PA5, PA7, PB0 a PB1,
// citac pulzu PA0.
// ---------------------------------------------------------------

#define PIN_TESTSIGNAL PB6

// Spusti generator. Vraci skutecne nastavenou frekvenci v Hz -
// delicky jsou celociselne, takze se pozadavek nemusi trefit presne.
uint32_t testSignalInit(uint32_t hz);

// Zastavi generator.
void testSignalStop();

// Skutecne nastavena frekvence.
uint32_t testSignalHz();
