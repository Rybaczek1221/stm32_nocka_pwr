# Plan rozbudowy: czujniki, Paint i telemetria

Ten dokument opisuje następny etap projektu **PWR Night Arcade**:

- podłączenie obu czujników,
- wykorzystanie wspólnej magistrali I2C,
- nową grę **PAINT**,
- osobny ekran telemetrii w czasie rzeczywistym,
- organizację przewodów na płytce stykowej.

Na tym etapie jest to plan integracji. Nie zmieniamy jeszcze logiki programu
ani konfiguracji CubeMX bez wcześniejszego sprawdzenia modułów na sprzęcie.

## Decyzja: wykorzystujemy oba czujniki

### MPU6050 - ruch i telemetria

MPU6050 będzie głównym czujnikiem ruchu:

- przyspieszenie X/Y/Z,
- prędkość kątowa X/Y/Z,
- wykrywanie przechyłu,
- wykrywanie potrząśnięcia,
- dane do ekranu telemetrii,
- opcjonalnie sterowanie efektami w Paint.

W Paint MPU6050 będzie poruszał całym kursorem, bez użycia pada, a czujnik
może również służyć do:

- zmiany koloru przez przechylenie w lewo/prawo,
- zwiększania grubości pędzla przez przechylenie góra/dół,
- uruchomienia „shake to clear” po potrząśnięciu.

### KAmodVL6180 - zmiana koloru gestem

VL6180 będzie czujnikiem odległości:

- zbliżenie dłoni zmienia kolor,
- kolejne zbliżenia przechodzą przez paletę,
- telemetryka pokazuje odległość i stan pomiaru,
- może działać jako dodatkowy gest bez wciskania przycisku.

Do pierwszej wersji Paint rekomendowane jest użycie MPU6050 do sterowania
obu osi. VL6180 pozostaje opcjonalnym rozszerzeniem do gestów i zmiany
koloru.

## Ile pinów potrzebujemy

Oba czujniki korzystają ze wspólnej magistrali I2C. Nie potrzebujemy osobnych
pinów dla każdego urządzenia.

| Funkcja | Liczba pinów |
|---|---:|
| I2C SCL wspólne dla MPU6050 i VL6180 | 1 |
| I2C SDA wspólne dla MPU6050 i VL6180 | 1 |
| VCC | wspólna szyna zasilania |
| GND | wspólna szyna masy |
| Dodatkowe GPIO dla Paint | 0 |
| Dodatkowe GPIO dla telemetrii | 0 |

Łącznie oba czujniki wymagają **2 pinów sygnałowych STM32**:

- `PB8` - `I2C1_SCL`,
- `PB9` - `I2C1_SDA`.

W obecnym układzie te piny nie są używane przez TFT, joystick ani diody
życia. Paint również nie potrzebuje nowych pinów, ponieważ korzysta z już
podłączonego joysticka, przycisku i TFT.

## Aktualny układ pinów

Poniższa tabela pokazuje, czego nie należy przepinać.

| Element | Pin STM32 | Rola |
|---|---|---|
| Joystick VRx | PA0 | ADC1_IN5 |
| Joystick VRy | PA1 | ADC1_IN6 |
| Joystick SW | PB0 | przycisk, pull-up |
| TFT MOSI | PC3 | SPI2 MOSI |
| TFT SCK | PB10 | SPI2 SCK |
| TFT DC | PB11 | dane/komenda TFT |
| TFT CS | PB12 | wybór TFT |
| TFT RST | PB2 | reset TFT |
| LED życia 1 | PC6 | wyjście GPIO |
| LED życia 2 | PC7 | wyjście GPIO |
| LED życia 3 | PC8 | wyjście GPIO |
| LED życia 4 | PC9 | wyjście GPIO |
| LED życia 5 | PC10 | wyjście GPIO |
| I2C SCL | PB8 | nowy wspólny sygnał |
| I2C SDA | PB9 | nowy wspólny sygnał |

`PA5` pozostaje używany przez wbudowaną diodę LD2.

## Jak podłączyć oba czujniki

### Wspólna magistrala

Połączenie wykonujemy równolegle:

```text
STM32 PB8 / SCL ───── SCL MPU6050
                  └── SCL VL6180

STM32 PB9 / SDA ───── SDA MPU6050
                  └── SDA VL6180

STM32 3V3 ─────────── VCC MPU6050
                  └── VCC VL6180

STM32 GND ─────────── GND MPU6050
                  └── GND VL6180
```

### Tabela połączeń

| STM32 / szyna | MPU6050 Crowtail | KAmodVL6180 |
|---|---|---|
| 3V3 | VCC | VCC zgodne z opisem modułu |
| GND | GND | GND |
| PB8 / I2C1_SCL | SCL | SCL |
| PB9 / I2C1_SDA | SDA | SDA |

Nie łączymy `INT`, `FSYNC`, `XDA` ani `XCL` MPU6050, jeżeli konkretna wersja
modułu ich wymaga do specjalnych funkcji. Do pierwszego uruchomienia
wystarczą VCC, GND, SCL i SDA.

Typowe adresy 7-bitowe:

- MPU6050: `0x68`, ewentualnie `0x69` po zmianie AD0,
- VL6180: `0x29`.

Przed implementacją należy wykonać skan I2C i potwierdzić adresy. Różne
wersje płytek mogą mieć inne podciąganie lub domyślną konfigurację adresu.

## Organizacja płytki stykowej

Na płytce stykowej wystarczy przygotować wspólne szyny:

1. `3V3` z Nucleo podłącz do czerwonej szyny zasilania.
2. `GND` z Nucleo podłącz do niebieskiej szyny masy.
3. VCC obu czujników podłącz do szyny `3V3`.
4. GND obu czujników podłącz do szyny `GND`.
5. Z PB8 poprowadź jedną linię do SCL MPU6050 i drugą krótką gałąź do SCL
   VL6180.
6. Z PB9 poprowadź jedną linię do SDA MPU6050 i drugą krótką gałąź do SDA
   VL6180.
7. Pięć LED pozostaje na PC6-PC10: PC6-PC7 czerwone, PC8 żółta,
   PC9-PC10 zielone.
8. Każda LED musi mieć osobny rezystor szeregowy, np. 330 ohm.

Nie wkładamy sygnałów SDA/SCL do szyny zasilania. Przewody I2C powinny być
krótkie i prowadzone z dala od przewodów SPI TFT, gdy obraz ma pracować przy
wyższym zegarze.

### Kontrola napięć

Przed podłączeniem sprawdź oznaczenia na konkretnych płytkach. Jeżeli moduł
ma wejścia 5 V tolerant lub własny regulator, nadal najbezpieczniej zasilać
go z 3V3, o ile dokumentacja modułu na to pozwala. Nie podawaj 5 V na
bezpośrednie linie GPIO STM32.

## Projekt gry PAINT

### Sterowanie

- MPU6050 - ruch kursora w obu osiach względem pozycji płaskiej,
- przycisk joysticka - pauza,
- VL6180 - zmiana koloru po wykryciu zbliżenia dłoni,
- MPU6050 - opcjonalna zmiana grubości pędzla przez przechył,
- dłuższe przytrzymanie przycisku - wyczyszczenie ekranu po potwierdzeniu.

### Proponowana paleta

Paint powinien używać kolorów widocznych na TFT:

1. cyjan,
2. żółty,
3. pomarańczowy,
4. czerwony,
5. fioletowy,
6. biały.

Kolor aktualny i rozmiar pędzla powinny być pokazane w górnym pasku. Obszar
rysowania musi mieć własną ramkę, aby użytkownik nie malował po HUD.

### Tryby Paint

Pierwsza wersja powinna mieć dwa proste tryby:

- **RYSOWANIE** - kursor zostawia ślad podczas ruchu,
- **GUMKA** - kursor maluje kolorem tła.

Zmiana koloru przez VL6180 powinna mieć histerezę i blokadę czasową, np.
jedna zmiana maksymalnie co 400 ms. Zapobiega to przeskakiwaniu wielu
kolorów, gdy dłoń pozostaje blisko czujnika.

## Osobne menu telemetrii

W menu **CZUJNIKI** są telemetria, sensorowy Paint, gra BALANS oraz ekran
LED-ów. Ekran testu jest odświeżany
bez pełnego czyszczenia TFT.

### Stabilizacja odczytu MPU6050

Sterownik budzi układ, ustawia zakres akcelerometru na +/-2 g, filtr
dolnoprzepustowy oraz próbkowanie około 200 Hz. Odczyt ma timeout 20 ms i
automatycznie próbuje oba typowe adresy `0x68` i `0x69`. W grach pozycja
neutralna jest średnią 5 próbek z płytki leżącej płasko, a bieżący pomiar
przechodzi przez filtr IIR 1/4 oraz strefę martwą. Dzięki temu kropka nie
drży przy nieruchomej płytce i nie ucieka od pojedynczego zakłócenia.

### Dane MPU6050

```text
MPU6050
ACC X: ...
ACC Y: ...
ACC Z: ...
GYR X: ...
GYR Y: ...
GYR Z: ...
```

Wartości należy pokazywać jako surowe odczyty albo przeliczone jednostki
z opisem zakresu. Na dole można dodać:

- `TILT: LEFT/RIGHT/UP/DOWN`,
- `SHAKE: YES/NO`,
- licznik odświeżeń.

### Dane VL6180

```text
VL6180
DIST: ... mm
LIGHT: ...
STATUS: OK/ERROR
```

W przypadku timeoutu lub błędu I2C ekran musi pokazać `ERROR`, a nie udawać
poprawnego pomiaru. To ułatwi diagnostykę na pokazie.

### Odświeżanie

Rekomendowana częstotliwość:

- odczyt MPU6050: 20-50 Hz,
- odczyt VL6180: 10-20 Hz,
- odświeżenie tekstu telemetrii: 5-10 Hz.

Odczyty powinny być wykonywane z timeoutem HAL. Nie wolno blokować głównej
pętli przez nieskończone oczekiwanie na I2C.

## Kolejność implementacji

1. Fizycznie podłączyć tylko MPU6050 do PB8/PB9.
2. Włączyć I2C1 w CubeMX i sprawdzić skan adresów.
3. Dodać sterownik odczytu MPU6050 i ekran telemetrii.
4. Podłączyć VL6180 równolegle i potwierdzić `0x29`.
5. Dodać odczyt odległości oraz zmianę koloru.
6. Dodać wejście `CZUJNIKI` i podmenu testu, Paint, BALANS oraz LED-ów.
7. Sterować PAINT wyłącznie akcelerometrem MPU6050.
8. Zaimplementować ramkę, HUD, paletę i rysowanie.
9. Dodać filtrację, histerezę i obsługę błędów I2C.
10. Przetestować pokazowo: menu, Paint, telemetrię, gry i diody życia.

## Kryteria gotowości

- oba adresy I2C są widoczne na skanerze,
- telemetria pokazuje zmiany przy poruszaniu płytką,
- odległość VL6180 zmienia się przy zbliżaniu dłoni,
- Paint nie reaguje na joystick i nie drży przy płaskiej płytce,
- BALANS wykorzystuje wyłącznie dane MPU6050 i nalicza punkty za precyzję,
- rysowanie nie narusza HUD ani ramki,
- błąd I2C jest widoczny na ekranie,
- istniejące gry i pięć LED życia działają tak samo jak wcześniej.
