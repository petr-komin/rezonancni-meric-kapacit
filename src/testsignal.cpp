#include <Arduino.h>
#include "testsignal.h"

static uint32_t skutecnaHz = 0;

uint32_t testSignalInit(uint32_t hz) {
  if (hz == 0) return 0;

  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

  // PB6 jako vystup alternativni funkce, push-pull, 50 MHz.
  // V CRL zabira pin 6 bity 24..27: MODE=11, CNF=10 -> 0xB
  GPIOB->CRL = (GPIOB->CRL & ~(0xFUL << 24)) | (0xBUL << 24);

  // Najdeme delicky tak, aby 72 MHz / (PSC+1) / (ARR+1) = hz
  // a ARR bylo co nejvetsi, kvuli presnosti stridy.
  uint32_t psc = 0;
  uint32_t deleni = 72000000UL / hz;
  while (deleni > 65536UL) {
    psc++;
    deleni = 72000000UL / (psc + 1) / hz;
  }
  if (deleni < 2) deleni = 2;
  uint32_t arr = deleni - 1;

  TIM4->CR1 = 0;
  TIM4->PSC = psc;
  TIM4->ARR = arr;
  TIM4->CCR1 = (arr + 1) / 2;  // strida 50 %
  TIM4->CCMR1 = (6UL << 4) | TIM_CCMR1_OC1PE;  // PWM rezim 1
  TIM4->CCER = TIM_CCER_CC1E;
  TIM4->EGR = TIM_EGR_UG;
  TIM4->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;

  skutecnaHz = 72000000UL / (psc + 1) / (arr + 1);
  return skutecnaHz;
}

void testSignalStop() {
  TIM4->CR1 = 0;
  TIM4->CCER = 0;
  skutecnaHz = 0;
}

uint32_t testSignalHz() { return skutecnaHz; }
