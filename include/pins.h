#pragma once
// ---------------------------------------------------------------
// Zapojeni displeje 1.44" SPI 128x128 V1.1 (radic ST7735S)
// na Blue Pill STM32F103C8, hardwarove SPI1
// ---------------------------------------------------------------
//  Displej      Blue Pill
//  -------      ---------
//  VCC   ....   3V3      (jen se spajenou propojkou pres stabilizator!)
//  GND   ....   G
//  GND   ....   G
//  NC    ....   -
//  NC    ....   -
//  CLK   ....   PA5   (SPI1_SCK)   - na nekterych modulech znaceno SCK / SCL
//  SDA   ....   PA7   (SPI1_MOSI)  - na nekterych modulech znaceno DIN / MOSI
//  RS    ....   PB1   - datovy/prikazovy vyber, jinde znaceno A0 / DC
//  RST   ....   PB0   - reset displeje
//  CS    ....   PA4   (SPI1_NSS)
//  (BL / LED / BLK) .. 3V3 pres 100R, nebo primo 3V3

#define TFT_SCLK  PA5   // jen pro dokumentaci, HW SPI1 si pin bere sam
#define TFT_MOSI  PA7   // jen pro dokumentaci, HW SPI1 si pin bere sam
#define TFT_CS    PA4
#define TFT_DC    PB1   // = RS / A0
#define TFT_RST   PB0

#define TFT_W     128
#define TFT_H     128

// LED na desce Blue Pill (sviti pri LOW)
#define LED_ONBOARD PC13

// Nulovaci tlacitko - spina na zem, vyuziva vnitrni pull-up.
// PB5 je volny; PB3, PB4 a PA15 ne, ty jsou vychozi JTAG a musely
// by se nejdriv odmapovat.
#define PIN_NULOVANI PB5
