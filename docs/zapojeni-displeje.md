# Zapojení displeje 1.44" SPI 128x128 V1.1 (ST7735S)

Modul má 8 vývodů na jedné straně (typické značení červené destičky "1.44 TFT SPI V1.1"):

| Pin displeje | Jiné značení | Blue Pill | Poznámka |
|---|---|---|---|
| GND | | G | zem |
| VCC | | **3V3** | jen se spájenou propojkou přes stabilizátor, viz níže |
| SCL | SCK, CLK | **PA5** | SPI1_SCK, hardwarové SPI |
| SDA | MOSI, DIN | **PA7** | SPI1_MOSI |
| RES | RST | **PB0** | reset displeje |
| DC | A0, RS | **PB1** | výběr data/příkaz |
| CS | | **PA4** | SPI1_NSS |
| BLK | LED, BL | **3V3** | podsvícení; některé kusy potřebují naopak GND |

MISO se nepoužívá (z displeje se nečte), PA6 zůstává volné.

## Poznámky

* **BLK:** na většině modulů V1.1 je podsvícení zapnuté už bez připojení BLK
  (má pull-up). Když displej nesvítí, zkus BLK na 3V3, a pokud ani to ne, na GND.
  Přes 100 Ω odpor, ať se netrápí pin.
* **Napájení (POZOR):** tenhle kus má na zadní straně stabilizátor 3,3 V
  (SOT-23, tři nožičky) a vedle něj pájecí plošky, které ho přemostí.

  | Propojka | VCC | Pozn. |
  |---|---|---|
  | **spájená** | **3,3 V** | tahle deska; řadič napájen napřímo |
  | rozpojená | 5 V | přes stabilizátor, původní stav |

  **Nikdy nedávej 5 V se spájenou propojkou** — šlo by přímo na řadič a zničilo
  by ho.

  Bez propojky a na 3,3 V řadič nenastartuje: stabilizátor má úbytek ~1 V, na
  řadič zbude něco přes 2 V. Podsvícení přitom svítí dál, protože pin LED jde na
  diody napřímo mimo regulátor. Poznávací znamení je, že se signály na vstupech
  ořezávají pod 3,3 V, jak je stahují ochranné diody podvyživeného řadiče.
* **Logika zůstává 3,3V.** Na signálové piny 5 V nepouštěj; řadič si svoje
  3,3 V vyrábí sám a 3,3V logiku přijme bez problémů.
* Pokud jsou obraz nebo barvy rozhozené (šum, posunutý obraz), sniž rychlost SPI
  v `setup()` z 24 MHz na 8 MHz — dlouhé propojky to potřebují.

## ST-Link V2 → Blue Pill

| ST-Link | Blue Pill (4pin SWD konektor) |
|---|---|
| SWCLK | SWCLK (DCLK) |
| SWDIO | SWDIO (DIO) |
| GND | GND |
| 3.3V | 3.3V |

Jumpery BOOT0 i BOOT1 nechej v poloze 0 (normální běh z flash).

## Sériový výpis (volitelné)

USART1 na PA9 (TX) / PA10 (RX), 115200 Bd. Stačí USB-UART převodník:
PA9 → RX převodníku, PA10 → TX převodníku, společná zem. Pak `pio device monitor`.


## Geometrie panelu

Ověřeno měřením: **128×160, posun počátku (0, 0)**, varianta řadiče odpovídá
1,8" modulu.

Výška 160 je bezpečná i pro menší panel 128×128 — přebytečné řádky se zapíšou
mimo viditelnou oblast a nic nezkazí. Opačná chyba by ukrojila spodek obrazu.

Kalibrační režim (`pio run -e kalibrace -t upload`) střídá dva obrazy:

1. celá paměť řadiče červeně — co zůstane červené, tam se nekreslí
2. naše okno černě, **bílý proužek na prvním řádku**, **zelený na posledním**

Geometrie sedí, když bílý proužek leží na úplném horním okraji panelu, zelený
na úplném spodním a červená zmizí beze zbytku.

## Vadný modul

Pozor, jeden z modulů 1,44" 128×128 je **vadný**. Projevuje se tak, že se
chování mění mezi jednotlivými běhy bez odpovídající příčiny v kódu: jednou
plní celou plochu barvami, podruhé projde jen tenká přerušovaná čárka, která
se mezi cykly posouvá. Napájení, podsvícení i reakce na RESET přitom fungují.

Když se displej chová nevysvětlitelně a příznaky skáčou, **vyzkoušej jiný
modul dřív, než začneš ladit software**. Stálo nás to hodně času.

## Diagnostika pomocí ST-Link

Hodnoty se dají číst z RAM i bez sériového portu:

```bash
arm-none-eabi-nm .pio/build/bluepill_f103c8/firmware.elf | grep g_pulzy
```

```
OCD=$HOME/.platformio/packages/tool-openocd
$OCD/bin/openocd -s $OCD/openocd/scripts -f interface/stlink.cfg \
  -c "set CPUTAPID 0x2ba01477" -f target/stm32f1x.cfg \
  -c "init" -c "halt" -c "mdw <adresa>" -c "resume" -c "exit"
```

Užitečné proměnné: `g_pulzy`, `g_hz`, `g_cyklu`, `g_faze`, `g_msSnimek`,
`g_isrBrana`, `g_isrPreteceni`.
