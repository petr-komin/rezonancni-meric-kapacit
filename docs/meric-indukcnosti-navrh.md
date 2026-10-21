# Měřič indukčnosti — návrh

Rozpracováno, nepostaveno. Čítač i displej jsou společné s měřičem kapacit,
liší se oscilátor, vstupní pin a vzorec.

Cíl: **VKV cívky, 10 nH až 1 mH**.

## Topologie: jednočinný Colpitts, ne dvojčinný

U měřiče kapacit je dvojčinné zapojení se dvěma J310 správně. Pro indukčnosti
ne — a důvod je v tom, **kde skončí měřicí svorky**.

Měřená cívka se zapojuje do **série**, takže musí být někde v laděném obvodu.
V dvojčinném zapojení jsou oba konce cívky horké, kmitají plnou amplitudou
proti zemi. Svorky by tedy byly na vysokém vf napětí a každá parazitní
kapacita — přívody, ruka, kryt — by zatěžovala laděný obvod. Jediné studené
místo je střed, jenže ten drží střídavou zem a napájení, takže ho přerušit
nelze.

Jednočinný Colpitts má **studený konec u země**. Napětí na měřené cívce je
jen `Lx/(Lref+Lx)` z celkového — při 100 nH proti 2 µH jsou to 5 %. Parazitní
kapacita svorek tím ztrácí vliv zhruba dvacetkrát. Jedna svorka je navíc
rovnou zem, což usnadní stínění.

Zpětná vazba se dělí kapacitně, takže **odbočka na cívce odpadá**.

## Zapojení

| Součástka | Hodnota | Poznámka |
|---|---|---|
| `Lref` | 2 µH | ~26 závitů, žlutý toroid, **bez odbočky** |
| `C1`, `C2` | 1 nF C0G | v sérii dají 500 pF |
| zdrojový odpor | ~1 kΩ | |
| návrat hradla | 1 MΩ na zem | pro rozpojené svorky |
| drain | tlumivka 47 µH nebo odpor | |
| tvarovač | 74AC04 + 1 MΩ zpětně | obdélník rovnou na vstup |

Cívka jde z **hradla** na zem, ne z drainu. Hradlem neteče proud, takže se
měřenou cívkou nešíří stejnosměrný proud a její odpor je jedno. Odpadá tím
potíž s napájením přes měřenou cívku.

Frekvence nakrátko vyjde **5,03 MHz**.

## Rozsah

| Měřená cívka | Frekvence | Posun |
|---|---|---|
| zkrat | 5033 kHz | — |
| 10 nH | 5021 kHz | 12,5 kHz |
| 50 nH | 4971 kHz | 62 kHz |
| 100 nH | 4912 kHz | 121 kHz |
| 500 nH | 4502 kHz | |
| 1 µH | 4109 kHz | |
| 10 µH | 2055 kHz | |
| 100 µH | 705 kHz | |
| 1 mH | 225 kHz | |

Rozlišení kolem **0,03 nH** při driftu 8 ppm. Na padesátinanohenrové cívce
je to šest setin procenta.

### Proč zrovna 2 µH

`Lref` ovlivňuje jen dolní konec rozsahu — u velkých cívek se v součtu ztratí.
Její zmenšení tedy horní hranici nesníží, ale dolní výrazně zlepší. Proti
dříve zvažovaným 5 µH je rozlišení na malých cívkách čtyřikrát lepší.

Dolní mez je dána tím, že `Lref` musí převládnout nad indukčností přípravku
(přívody a svorky mají snadno 30 nH).

## Co rozsah omezuje nahoře

Ne obvod — ten by šel dál. Limitem je **vlastní kapacita měřené cívky**.
Pod svou vlastní rezonancí se cívka jeví indukčnější:

```
L_zdánlivá = L / (1 − (f/f_vl)²)
```

Po dosazení vyjde, že u velkých cívek, kde `Lx` převládne nad `Lref`, se
indukčnost vykrátí:

```
f / f_vl ≈ √(Cp / Ctank)
```

Záleží tedy **jen na poměru vlastní kapacity cívky k té v obvodu**, ne na
indukčnosti. Chyba je napříč rozsahem zhruba stejná a zmenší se zvětšením
`Ctank`.

| Cívka | `Cp` | Chyba při `Ctank` = 500 pF |
|---|---|---|
| VKV, pár závitů | ~1 pF | +0,2 % |
| tlumivka 1 mH | ~12 pF | +2,4 % |

Kdyby bylo potřeba jít až k 10 mH, zvětšit `C1` a `C2` na 2,2 nF.

## Vstupní pin — přepínání bez přepínače

TIM2 umí brát hodiny ze dvou různých pinů. Přepnutí je jediný zápis do
`SMCR`, žádný přepínač v signálové cestě.

| Zdroj | Pin | Režim | Pro co |
|---|---|---|---|
| ETR | **PA0** | externí hodiny 2 (`ECE`) | kapacity |
| TI2 | **PA1** | externí hodiny 1 (`SMS=111`, `TS=110`) | indukčnosti |

```c
// kapacity na PA0, s děličkou kvůli 35 MHz
TIM2->SMCR = TIM_SMCR_ECE | (etps << 12) | (etf << 8);

// indukčnosti na PA1
TIM2->CCMR1 = (0x1 << 8) | (0x3 << 12);   // CC2S = TI2, filtr
TIM2->SMCR  = (0x6 << 4) | 0x7;           // TS = TI2FP2, SMS = ext. hodiny 1
```

**Dělička je jen na vstupu ETR.** Tu potřebují kapacity, protože 35 MHz je na
hranici čítače. Indukčnosti běží na 5 MHz, kde potřeba není. Proto kapacity
na PA0 a indukčnosti na PA1, obráceně by to nešlo.

PA1 stejně jako PA0 **není pětivoltový**.

Alternativa: `ETR` se dá přemapovat z PA0 na PA15 zápisem do `AFIO_MAPR`.
PA15 je po resetu JTAG, který by se musel vypnout — SWD zůstane funkční.
Zmíněno pro případ, že by oba oscilátory běžely vysoko.

## Napájet vždy jen jeden oscilátor

Dva oscilátory v jedné krabičce se mohou vzájemně rušit a v nejhorším případě
se přitáhnout na společný kmitočet. Poznalo by se to podle skákající nebo
zaseknuté hodnoty.

Přepínač režimu by proto měl spínat **napájení**, ne signál. Mechanický
přepínač ve vf cestě má kapacitu, indukčnost přívodů a přechodový odpor.
Stačí pin procesoru a malý P-MOSFET nebo PNP na každou větev.

Nejlépe přečíst polohu přepínače na volném pinu a podle ní nastavit zdroj
hodin, napájení oscilátorů i jednotky na displeji — pak nejde přepnout
hardware a zapomenout software.

## Výpočet

```
Lx = Lref · ((f₀/f)² − 1)
```

`f₀` je frekvence **nakrátko**, ne naprázdno. U kapacit se nuluje
s rozpojenými svorkami, protože měřená kapacita je paralelně. U indukčností
je to obráceně — měřená cívka je v sérii, takže nulovým stavem je zkrat.

To je důležitější, než to zní: **10 nH je zhruba centimetr drátu**, takže
přívody přípravku mají samy o sobě desítky nanohenry. Bez vynulování nakrátko
by se měřil hlavně přípravek.

### Odkud se vezme Lref

Z kalibrace kapacit, stejným slídovým kondenzátorem. Směrnice `a` dá

```
L = a / (4π² · 10⁻¹²)
```

Referenční indukčnost tedy **není potřeba nikde měřit** — vypadne z kalibrace
známým kondenzátorem. Viz [mereni-kapacity.md](mereni-kapacity.md).

## Co zbývá udělat

- [ ] Postavit oscilátor
- [ ] Navinout `Lref`, ~26 závitů na žlutém toroidu
- [ ] Spínání napájení obou oscilátorů
- [ ] Přepínač režimu na volný pin
- [ ] Software: přepínání zdroje hodin, výpočet, jednotky nH/µH
- [ ] Nulování podle režimu rozlišit: zkrat vs. rozpojeno
