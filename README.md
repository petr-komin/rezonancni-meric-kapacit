# Rezonanční měřič kapacit

STM32F103C8 (Blue Pill) + displej 1.44" SPI 128x128 (ST7735S), PlatformIO.

Stav: **měření frekvence i kapacity funguje**, zkalibrováno slídovým 62 pF
(±2 %). Měřič indukčnosti je navržený, nepostavený.

## Zapojení

* Displej — [docs/zapojeni-displeje.md](docs/zapojeni-displeje.md)
* Měření frekvence — [docs/mereni-frekvence.md](docs/mereni-frekvence.md)
* Měření kapacity — [docs/mereni-kapacity.md](docs/mereni-kapacity.md)
* Měřič indukčnosti, návrh — [docs/meric-indukcnosti-navrh.md](docs/meric-indukcnosti-navrh.md)

Odvození kalibračních vzorců krok za krokem je v
[docs/mereni-kapacity.md](docs/mereni-kapacity.md).

## Piny

| Pin | Funkce |
|---|---|
| PA4, PA5, PA7, PB0, PB1 | displej (CS, SCK, SDA, RST, A0) |
| PA0 | vstup měřené frekvence — **max 3,3 V, není pětivoltový** |
| PB5 | **nulovací tlačítko**, spíná na zem |
| PB6 | výstup zkušebního signálu |
| PC13 | LED na desce |

## Sestavení a nahrání

```bash
pio run              # přeložit
pio run -t upload    # nahrát přes ST-Link V2
pio device monitor   # sériový výpis (USART1, PA9/PA10, 115200)
```

Pokud má tvůj klon Blue Pillu 128 kB flash, přepiš v `platformio.ini`
`board = bluepill_f103c8` na `board = bluepill_f103c8_128k`.

## Řešení potíží

### Nic nefunguje, LED na PC13 nebliká

Procesor pravděpodobně visí v USB přerušení. **Nikdy** nepoužívej
`-D PIO_FRAMEWORK_ARDUINO_ENABLE_CDC=0` — PlatformIO testuje jen přítomnost
toho řetězce, ne hodnotu, takže `=0` USB CDC naopak zapne. Na klonech Blue Pillu
(špatný pull-up 10k místo 1,5k na PA12) se USB donekonečna resetuje, zahltí CPU
a `setup()` s `loop()` se nerozeběhnou. Řešení: flag úplně vypustit.

Diagnostika přes ST-Link — kde procesor uvízl:

```bash
OCD=$HOME/.platformio/packages/tool-openocd
$OCD/bin/openocd -s $OCD/openocd/scripts -f interface/stlink.cfg \
  -c "set CPUTAPID 0x2ba01477" -f target/stm32f1x.cfg \
  -c "init" -c "halt" -c "reg pc" -c "exit"
```

Vypsanou adresu přelož na jméno funkce:

```bash
$HOME/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-addr2line \
  -f -e .pio/build/bluepill_f103c8/firmware.elf 0x08001ce0
```

`current mode: Thread` = kód běží normálně.
`current mode: Handler External Interrupt(n)` = visí v přerušení.

### OpenOCD hlásí `UNEXPECTED idcode: 0x2ba01477`

Deska není originál ST, ale klon (CKS32/CS32F103). Už ošetřeno v `platformio.ini`
přes `set CPUTAPID`. Na funkci to nemá vliv.

### Kontrola hodin

```
RCC_CR   (0x40021000) — bit 17 HSERDY, bit 25 PLLRDY musí být 1
RCC_CFGR (0x40021004) — bity [3:2] SWS = 0b10 znamená, že jedou hodiny z PLL
```

### Podsvícení svítí, ale na displeji nic není

Zkontroluj zadní stranu modulu. Když je na ní stabilizátor (SOT-23, tři
nožičky), řadič při 3,3 V na VCC nenastartuje, zatímco podsvícení svítí dál.
Řešení: spájet plošky, které stabilizátor přemostí (pak VCC = 3,3 V), nebo
nechat rozpojené a dát na VCC 5 V. Nikdy ne obojí zároveň. Podrobnosti v [docs/zapojeni-displeje.md](docs/zapojeni-displeje.md).

Diagnostické režimy firmwaru:

```bash
pio run -t upload                 # měření frekvence
pio run -e kalibrace -t upload    # hledání nejvyšší spolehlivé rychlosti SPI
```

Hodnoty se dají číst z RAM přes ST-Link i bez sériového portu — proměnné
`g_pulzy`, `g_hz`, `g_isrBrana`, `g_isrPreteceni`. Adresu zjistíš přes `nm`:

```bash
arm-none-eabi-nm .pio/build/bluepill_f103c8/firmware.elf | grep g_hz
```
