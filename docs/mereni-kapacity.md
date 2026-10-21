# Měření kapacity

## Princip

Měřená kapacita se připojuje **paralelně** k laděnému obvodu, takže se sčítá
s tím, co už v něm je:

```
f = 1 / (2π · √(L · (C0 + Cx)))
```

Po umocnění a převrácení je vztah **lineární**:

```
1/f² = 4π²L · (C0 + Cx)
```

Když označíme `A = 1/f²`, je `A = a·Cx + b`, tedy přímka. Dvě měření se známými
kondenzátory ji určí úplně:

```
a = (A₁ − A₂) / (C₁ − C₂)
b = A₁ − a·C₁
```

A z nich pak všechno ostatní:

| Veličina | Vzorec |
|---|---|
| měřená kapacita | `Cx = (A − b) / a` |
| kapacita obvodu naprázdno | `C0 = b / a` |
| indukčnost cívky | `L = a / (4π²·10⁻¹²)` |

**Indukčnost není potřeba znát ani měřit** — to je hlavní přednost rezonanční
metody. Vypadne z kalibrace sama jako vedlejší produkt.

## Kalibrace

Dvě měření se známými kondenzátory. První bod může být naprázdno (0 pF).
Naměřené hodnoty se zapíšou do `platformio.ini`:

```ini
build_flags =
    -D KAL_F1=35586500.0 -D KAL_C1=0.0
    -D KAL_F2=6763900.0  -D KAL_C2=220.0
```

Kondenzátory se musí **výrazně lišit**, jinak je výsledek citlivý na chyby
měření.

### Na čem stojí přesnost

Absolutní přesnost celého měřidla stojí a padá na přesnosti referenčního
kondenzátoru — všechny naměřené hodnoty i dopočítané `C0` a `L` se škálují
přímo s jeho skutečnou hodnotou.

Na kalibraci se hodí **slídový nebo C0G/NP0** s tolerancí 1 %. Typy **X7R
a Y5V jsou nevhodné**: tolerance i 20 %, silná teplotní závislost a u Y5V
i závislost na přiloženém napětí.

## Naměřené hodnoty obvodu

Hartleyův oscilátor s odbočkou uprostřed cívky, oddělovací sledovač:

| Veličina | Hodnota |
|---|---|
| C0 — kapacita naprázdno | 6,963 pF |
| L — indukčnost cívky | 2,910 µH |
| f naprázdno | 35,354 MHz |

Kalibrováno kondenzátorem 139 pF s tolerancí 1 %. Absolutní přesnost stupnice
je tedy ±1 %.

### Historie kalibrace

Stojí za zapamatování, protože ukazuje, jak se chyba etalonu propíše do všeho.

| Etalon | Značeno | Skutečnost | Co to udělalo |
|---|---|---|---|
| modrá disková keramika 1 kV | 220 pF | 179 pF | stupnice o 19 % vedle |
| slída | 62 pF ±2 % | 63,45 pF | stupnice o 2,3 % vedle |
| **současný** | **139 pF ±1 %** | — | ±1 % |

Chyba se pozná jedině měřením jiného kondenzátoru, do kalibrace nezapojeného.
Keramika se prozradila hned (slída 62 pF podle ní vycházela 76 pF), slída až
při měření přesnějšího kusu — ukazovalo 135,8 pF místo 139.

### Zahřátí — čekat deset minut

Oscilátor se po zapnutí tepelně usazuje. Změřeno záznamem frekvence po
dvaceti sekundách:

| Do pásma kolem konečné hodnoty | Trvalo |
|---|---|
| ±0,005 pF | 9,5 min |
| ±0,002 pF | 10 min |
| ±0,001 pF | 10,5 min |

Celkový posun za dobu zahřívání byl **35 kHz**, což v kapacitě dělá jen
**0,014 pF**. Na frekvenčním displeji to vypadá dramaticky, v měřené veličině
je to setina pikofaradu.

Průběh stojí za povšimnutí: frekvence nejdřív kousek povyroste a teprve pak
klesne a usadí se. Ten vzestup je pracovní bod tranzistorů, který se ustaví
rychleji než teplota toroidu — a materiál -6 s kladným teplotním součinitelem
pak frekvenci zatlačí zpátky dolů.

**Kalibrovat i nulovat tedy až po deseti minutách.** U kalibrace je to
důležitější než u nulování: její dva body se odečítají v různých okamžicích,
takže by se rozdíl teplot promítl rovnou do sklonu.

### Výměna konektoru

Po výměně připojovacího konektoru u kapacit bylo nutné překalibrovat. Konektor
přidal **0,24 pF**, takže `C0` vzrostlo z 6,72 na 6,955 pF.

Pozor na postup: z jednoho měření se nedá rozlišit, jestli se posunul sklon
nebo offset. Je potřeba **změřit i frekvenci naprázdno** a udělat plnou
dvoubodovou kalibraci, jinak se chyba etalonu schová do `C0` a vyjdou
nesmyslné hodnoty.

### Ověření

| Kondenzátor | Značeno | Naměřeno | Odchylka |
|---|---|---|---|
| slída (do kalibrace nevstupovala) | 150 pF ±5 % | **152,6 pF** | +1,7 % ✓ |
| modrá disková keramika 1 kV | 220 pF | **179 pF** | −19 % ✗ |

První řádek je **nezávislé ověření** — tenhle kondenzátor se kalibrace nijak
neúčastnil, a přesto sedí. Potvrzuje to i platnost vztahu mimo kalibrační body:
kalibrováno na 0 a 62 pF, ověřeno na víc než dvojnásobku.

Druhý řádek ukazuje, proč se disková keramika na kilovolt na kalibraci nehodí.
Bývá to materiál Y5V nebo Z5U s tolerancí klidně −20/+80 %. Původně se podle ní
kalibrovalo a protáhla celou stupnici — slída 62 pF podle ní vycházela 76 pF.

### Co měřidlo umí a neumí

* **Absolutní hodnota:** ±2 %, dané tolerancí etalonu. Zlepší se jedině lepším
  etalonem — slída 1 % nebo proměření současné na kalibrovaném můstku.
* **Rozlišení a opakovatelnost:** setiny pikofaradu. Drift oscilátoru se do
  kapacity promítá zanedbatelně, viz tabulka níže.

Porovnávání dvou kondenzátorů mezi sebou nebo hledání malého rozdílu je tedy
podstatně přesnější než odečet absolutní hodnoty.

## Proč drift nevadí

Oscilátor driftuje o stovky hertzů. Vypadá to hrozivě, dokud se to nepřepočítá
na kapacitu — závislost je mocninná, takže velký relativní posun frekvence
odpovídá malé změně kapacity:

| Měřená kapacita | Frekvence | Vliv driftu 300 Hz |
|---|---|---|
| 0 pF | 35,6 MHz | 0,0001 pF |
| 220 pF | 6,76 MHz | 0,02 pF |
| 1150 pF | 3,0 MHz | 0,23 pF |

Na 220 pF je to jedna setina procenta. Rozlišení tedy neomezuje ani drift,
ani čítač — omezuje ho jen přesnost referenčního kondenzátoru.

## Poznámky k oscilátoru

Signál ze sledovače je **sinus**, ne obdélník. Vstup PA0 se překlápí zhruba
kolem poloviny napájení, takže amplituda musí projít pod ~1 V a nad ~2 V.

Při malých kapacitách, kdy frekvence stoupá k 36 MHz, amplituda klesá a měření
může vypadnout. Dvě řešení:

* **Bez součástek:** připojit do obvodu natrvalo pevný kondenzátor (třeba
  100 pF). Frekvence naprázdno klesne asi na 9 MHz, kde signál stačí.
  Rozlišení tím neutrpí — jeden pikofarad tam posune frekvenci o ~42 kHz,
  zatímco čítač rozlišuje 2 Hz. Po této úpravě je nutné překalibrovat.
* **Se součástkou:** za sledovač dát tvarovač. Na 36 MHz musí být rychlý —
  řada HC nestačí, **74AC04** nebo **74AC14** ano. Napájený z 3,3 V, vstup přes
  vazební kondenzátor a odpor 1 MΩ z výstupu na vstup, který ho sám posadí
  doprostřed.
