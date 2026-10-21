# Měření frekvence

Pulzy se počítají **hardwarově**, procesor je nepočítá v software.

## Princip

| Časovač | Role |
|---|---|
| **TIM2** | Čítač pulzů. Běží v režimu externích hodin 2 (`ECE`) — jeho hodinovým zdrojem je přímo signál na pinu `ETR`. Procesor jen obsluhuje přetečení 16bitového čítače a dělá z něj 32bitový. |
| **TIM3** | Hradlovací perioda. Na jejím konci se čítač odečte a z rozdílu se spočítá frekvence. |
| **TIM4** | Generátor zkušebního signálu (volitelný, na ověření). |

Čítač se **nenuluje**, jen se bere rozdíl proti minulému odečtu. Tím se neztratí
žádný pulz na přelomu period a přetečení 32 bitů se v odečtu vyruší samo.

## Piny

| Pin | Funkce |
|---|---|
| **PA0** | vstup měřené frekvence (`TIM2_ETR`), stažený dolů |
| **PB6** | výstup zkušebního signálu (`TIM4_CH1`), střída 50 % |

Displej používá PA4, PA5, PA7, PB0 a PB1, takže se nic nepřekrývá.

## Rozlišení

Rozlišení je dané délkou hradlovací periody: při bráně 1000 ms vyjde 1 Hz,
při 100 ms 10 Hz. Delší brána měří jemněji, ale reaguje pomaleji.

Maximální brána je 6553 ms — `ARR` je 16bitový a krok je 0,1 ms.

## Proč vlastní tabulka vektorů

`HardwareTimer.cpp` z jádra stm32duino je vždy přilinkovaná a definuje si
`TIM2_IRQHandler` i `TIM3_IRQHandler` sama. Nelze je tedy definovat znovu —
kolidovaly by symboly.

Nechat obsluhu na jádru ale taky nejde: když v ní nenajde svůj objekt
`HardwareTimer`, **nesmaže příznak přerušení** a procesor se zahltí nekonečnou
smyčkou. A pokus jet přes `HardwareTimer` selhal jinak — hradlovací přerušení
se po dvou tiknutích přestalo volat, přestože časovač prokazatelně běžel dál
(`CR1` měl `CEN`, `CNT` postupoval, `UIF` se nastavoval).

Řešení je tabulka vektorů přesunutá do RAM se dvěma přepsanými položkami.
Jádro zůstane nedotčené a přerušení chodí přímo k nám. Stojí to 304 bajtů RAM.

## Ověření celého řetězce

Propoj **PB6 → PA0** drátkem. Měřička musí ukázat přesně tu frekvenci, kterou
generátor vydává (výchozí 10 kHz). Ověří to najednou čítač, hradlování
i přepočet, bez jakéhokoliv externího přístroje.
