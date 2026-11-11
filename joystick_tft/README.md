# PWR Night Arcade

Projekt demonstracyjny na płytkę **NUCLEO-L476RG** z mikrokontrolerem
**STM32L476RG**, analogowym joystickiem oraz kolorowym wyświetlaczem TFT
ST7735S. Celem projektu jest zbudowanie małej, samodzielnej konsoli
zręcznościowej na pokaz podczas **Nocy PWR**.

Na ekranie startowym wyświetlane są dane autora:

```text
JAROSLAW RYBAK
PWR
```

Sterowanie odbywa się jednym joystickiem. W menu wybiera się trzy gry, a
ruchem w prawo otwiera się osobny ekran **CZUJNIKI** z telemetrią MPU6050. Na ekranie
odczyt odświeża się raz na sekundę, aby wartości były czytelne:

1. **SNAKE** - większa plansza z widoczną kratką, wynikiem, czasem i
   bonusami punktowymi.
2. **KROPKA** - żółta kropka unika czerwonych, pseudolosowych bomb. Bomb
   przybywa wraz z czasem.
3. **LABIRYNT** - kropką trzeba przejść z punktu A do B bez dotknięcia ścian.
   Gra ma dziesięć poziomów, a każdy kolejny jest trudniejszy.
4. **CZUJNIKI** - test I2C oraz bieżące dane akcelerometru i żyroskopu z MPU6050.
   Ruchem w dół można uruchomić grę **SAMOLOT**.

## Podłączenie elementów

Wszystkie elementy należy zasilać z **3,3 V** i połączyć ze wspólną masą.
Nie podawać 5 V na wejścia ADC, wyświetlacz ani linie GPIO.

Pięć czerwonych diod życia jest podłączonych do PC6-PC10. Dodatkowy opis
czujników MPU6050 i VL6180 znajduje się w [SENSORS.md](./SENSORS.md).

### Joystick analogowy

| Pin modułu | NUCLEO-L476RG | Funkcja |
|---|---|---|
| GND | GND | wspólna masa |
| VCC | 3V3 | zasilanie modułu |
| VRx | PA0 / ADC1_IN5 | oś pozioma |
| VRy | PA1 / ADC1_IN6 | oś pionowa |
| SW | PB0 | przycisk, wejście z pull-up |

Przycisk joysticka jest aktywny stanem niskim. Program rozpoznaje tylko
moment naciśnięcia, dzięki czemu przytrzymanie przycisku nie powoduje
wielokrotnego uruchomienia gry.

### Wyświetlacz TFT ST7735S

| Pin TFT | NUCLEO-L476RG | Funkcja |
|---|---|---|
| VCC | 3V3 | zasilanie |
| GND | GND | wspólna masa |
| DIN / MOSI | PC3 | dane SPI2 |
| CLK / SCK | PB10 | zegar SPI2 |
| CS | PB12 | wybór układu |
| DC | PB11 | komenda albo dane |
| RST | PB2 | reset wyświetlacza |
| BL | nie podłączać | podświetlenie modułu |

Program używa orientacji obrazu **160 x 128 pikseli** oraz formatu kolorów
RGB565.

### Diody życia

Każda dioda ma osobny rezystor szeregowy, a katodę należy podłączyć do GND:

| Dioda | GPIO |
|---|---|
| Życie 1 | PC6 |
| Życie 2 | PC7 |
| Życie 3 | PC8 |
| Życie 4 | PC9 |
| Życie 5 | PC10 |

Po utracie życia jedna dioda gaśnie. Po utracie ostatniego życia wszystkie
diody wykonują krótki efekt migania.

## Obsługa

### Menu główne

- joystick góra/dół - wybór gry,
- joystick w prawo - ekran CZUJNIKI, w lewo - powrót do gier,
- na telemetrii joystick w dół - start gry SAMOLOT,
- przycisk joysticka - start,
- paleta interfejsu: granat, fiolet, cyjan i żółty,
- zaznaczona pozycja jest pokazana na osobnym panelu z fioletową ramką.

### Pauza

Wciśnięcie przycisku w trakcie gry otwiera ekran pauzy:

- **WZNOW** - powrót do gry,
- **MENU** - powrót do ekranu głównego.

Po zakończeniu gry ekran pokazuje wynik oraz czas działania rundy. Przycisk
wraca wtedy do menu.

## Zasady gier

### SNAKE

Plansza ma komórki 8 x 8 pikseli. Fioletowa ramka wyznacza granice, a
delikatna szara kratka pokazuje pola ruchu. Głowa węża jest cyjanowa, ciało
fioletowe, a zwykłe jedzenie pomarańczowe. Co pewien czas pojawia się żółty
bonus wart pięć punktów. Kolizja ze ścianą albo własnym ciałem kończy grę.

### KROPKA

Żółta kropka porusza się w obszarze gry. Czerwone bomby pojawiają się
pseudolosowo z góry, dołu, lewej i prawej strony. Po pewnym czasie
częstotliwość pojawiania się bomb rośnie, a kolizja kończy grę.

### LABIRYNT

Żółta kropka startuje w punkcie A i musi dotrzeć do niebieskiego punktu B.
Czerwone ściany kończą próbę natychmiast po dotknięciu. Po przejściu poziomu
pojawia się kolejny układ, rośnie liczba przeszkód, zmniejsza się cel i
skraca się limit czasu. Ukończenie poziomu dziesiątego kończy grę sukcesem.

### CZUJNIKI

Ekran wykonuje test `WHO_AM_I`, pokazuje status `I2C TEST OK/ERROR`, identyfikator
układu oraz liczbę poprawnie pobranych próbek. Pokazuje też `ACC X/Y/Z` oraz
`GYR X/Y/Z`. Błąd oznacza brak odpowiedzi czujnika albo błąd magistrali.

### SAMOLOT

Samolot jest sterowany przechyleniem MPU6050. Przechylenie do góry lub dołu
zmienia wysokość, a przechylenie w lewo lub prawo zmienia kierunek lotu.
Trzeba przelecieć przez przerwy między czerwonymi przeszkodami. Wynik rośnie
za każdą miniętą przeszkodę. Jeżeli MPU6050 chwilowo nie odpowiada, joystick
zastępczo steruje samolotem, co pozwala bezpiecznie sprawdzić samą grę.

## Struktura kodu

```text
joystick_tft/
├── Core/
│   ├── Inc/
│   │   │   ├── game.h          # stany aplikacji i publiczne API gry
│   │   ├── lives.h         # obsługa pięciu diod życia
│   │   ├── joystick.h      # typy kierunków i stan joysticka
│   │   └── tft_st7735.h    # API wyświetlacza i kolory RGB565
│   └── Src/
│       ├── main.c          # inicjalizacja HAL i główna pętla programu
│       ├── joystick.c      # odczyt ADC oraz przycisku joysticka
│       ├── game.c          # menu, gry, telemetria, pauza i generator losowy
│       ├── i2c.c           # konfiguracja I2C1 na PB8/PB9
│       ├── mpu6050.c       # odczyt WHO_AM_I i danych MPU6050
│       ├── lives.c         # stan żyć, diody i efekt końca
│       ├── tft_st7735.c    # inicjalizacja TFT i szybkie rysowanie
│       └── spi.c           # konfiguracja SPI2
├── Drivers/                # biblioteki CMSIS i STM32 HAL
└── joystick_tft.ioc        # konfiguracja CubeMX/CubeIDE
```

### Najważniejsze decyzje techniczne

- `main.c` wykonuje tylko inicjalizację, odczyt wejścia i przekazanie zdarzeń
  do modułu gry.
- `game.c` działa jako prosta maszyna stanów: menu, gra, pauza i koniec gry.
- Generator liniowy zapewnia pseudolosowe pozycje jedzenia, bomb i celów bez
  dodatkowej biblioteki.
- Rysowanie prostokątów korzysta z bufora całego wiersza, a nie z osobnej
  transmisji SPI dla każdego piksela.
- Tekst jest wysyłany znakami w jednym obszarze obrazu, co ogranicza liczbę
  operacji CS/DC i przyspiesza przejścia ekranów.
- SPI2 pracuje z preskalerem 2; przed użyciem należy sprawdzić jakość obrazu
  na konkretnym module i przewodach.

## Uruchomienie

1. Otwórz projekt `joystick_tft` w STM32CubeIDE.
2. Sprawdź podłączenie 3,3 V, GND, SPI oraz joysticka.
3. Wybierz **Project > Build Project**.
4. Uruchom program przez **Debug** albo **Run**.
5. Po resecie powinno pojawić się menu główne.

Jeżeli obraz jest niestabilny, należy najpierw sprawdzić długość przewodów,
wspólną masę i napięcie 3,3 V, a następnie ewentualnie zmniejszyć
częstotliwość SPI w `Core/Src/spi.c` oraz w konfiguracji `joystick_tft.ioc`.

## Bezpieczeństwo

- Zawsze wyłącz zasilanie przed zmianą przewodów.
- Nie podłączaj joysticka do 5 V, ponieważ jego wyjścia trafiają bezpośrednio
  do ADC mikrokontrolera.
- Każda zewnętrzna dioda LED, jeżeli jest używana, musi mieć rezystor.
- Nie podłączaj pinu BL TFT do 5 V; w tym module pozostaje niepodłączony.
