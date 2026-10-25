# Projekt sterownika pojazdu i mini gry

## 1. Cel projektu

Projekt jest realizowany na płytce **NUCLEO-L476RG** z mikrokontrolerem
**STM32L476RG** oraz elementach z zestawu FORBOT STM32L4.

Docelowo program ma połączyć:

- kolorowy wyświetlacz TFT Waveshare 1.8" 128x160 ze sterownikiem ST7735S,
- analogowy joystick dwuosiowy z przyciskiem,
- diody LED pokazujące kierunek wychylenia joysticka,
- prostą grę sterowaną joystickiem,
- w dalszym etapie czujniki I2C, np. MPU6050 i APDS-9960,
- elementy sterujące pojazdem, np. mostek H i silniki.

Pierwszym celem nie jest jeszcze kompletna gra, ale podział projektu na
czytelne moduły i sprawdzenie kolejno każdego elementu sprzętowego.

## 2. Plan kolejnych etapów

1. Sprawdzenie uruchamiania mikrokontrolera i diody LD2.
2. Odczyt przycisku użytkownika B1 z programowym debounce'em.
3. Uruchomienie SPI1 i test komunikacji z TFT.
4. Inicjalizacja sterownika ST7735S i wyświetlenie jednolitego koloru.
5. Dodanie podstawowych funkcji graficznych: punkt, prostokąt, tekst.
6. Odczyt osi joysticka przez ADC oraz odczyt przycisku SW.
7. Wskaźnik położenia joysticka na diodach LED w układzie 3x3.
8. Demo: poruszany punkt lub pojazd na ekranie TFT.
9. Prosta gra, np. omijanie przeszkód albo mini Snake.
10. Rozbudowa o czujniki i sterowanie pojazdem.

## 3. Sprzęt

### 3.1. Płytka główna

- NUCLEO-L476RG,
- mikrokontroler STM32L476RG,
- rdzeń ARM Cortex-M4,
- poziomy logiczne GPIO: 3,3 V,
- zasilanie modułów peryferyjnych: preferowane 3,3 V.

### 3.2. Wyświetlacz

Użyty moduł to **Waveshare 1.8inch LCD Module**, rozdzielczość
**128x160 pikseli**, kontroler **ST7735S**.

Wyświetlacz korzysta z interfejsu SPI i nie posiada magistrali MISO
potrzebnej do podstawowego wysyłania grafiki. STM32 wysyła do niego dane
przez `MOSI`, zegar `SCK` oraz sygnały sterujące GPIO.

### 3.3. Joystick

Moduł joysticka ma wyprowadzenia:

- `GND` - masa,
- `+5V` - zasilanie modułu,
- `VRx` - napięcie analogowe osi X,
- `VRy` - napięcie analogowe osi Y,
- `SW` - przycisk joysticka.

W projekcie z STM32 należy najpierw zasilić moduł z **3V3**, a nie z `+5V`,
aby napięcie z wyjść `VRx` i `VRy` nie przekroczyło dopuszczalnego napięcia
wejść analogowych mikrokontrolera. Ostatecznie należy sprawdzić napięcie
multimetrem w skrajnych położeniach joysticka.

## 4. Proponowana mapa połączeń

Poniższa mapa jest wersją roboczą dla pierwszego prototypu. Przed
podłączeniem zasilania należy porównać ją z opisami na płytce oraz
sprawdzić, czy wybrane piny nie są zajęte przez inną funkcję.

### 4.1. TFT ST7735S

| Pin TFT | Funkcja | STM32L476RG | Opis |
|---|---|---|---|
| VCC | zasilanie | 3V3 | nie podłączać do 5 V |
| GND | masa | GND | wspólna masa |
| DIN | dane SPI | PA7 | SPI1_MOSI |
| CLK | zegar SPI | PA5 | SPI1_SCK |
| CS | wybór układu | PB6 | GPIO, stan aktywny niski |
| DC | dane / komenda | PB7 | GPIO |
| RST | reset TFT | PB8 | GPIO |
| BL | podświetlenie | 3V3 | stałe podświetlenie w prototypie |

Wariant z osobnym sterowaniem `BL` przez tranzystor można dodać później.
Na pierwszym etapie podświetlenie jest włączone na stałe.

### 4.2. Joystick

| Pin joysticka | STM32L476RG | Funkcja |
|---|---|---|
| GND | GND | masa |
| +5V / VCC | 3V3 | zasilanie modułu w projekcie STM32 |
| VRx | PA0 / A0 | ADC, oś X |
| VRy | PA1 / A1 | ADC, oś Y |
| SW | PB0 | wejście cyfrowe z pull-up, aktywne niskim stanem |

Przycisk `SW` powinien być obsługiwany z debounce'em, podobnie jak
przycisk B1 na płytce Nucleo.

### 4.3. Diodowy wskaźnik 3x3

Proponowany wskaźnik składa się z dziewięciu osobnych diod LED. Każda
dioda musi mieć własny rezystor szeregowy, typowo **220-1 kOhm**.
Nie należy łączyć diod bez rezystorów.

| Pozycja | Proponowany GPIO | Znaczenie |
|---|---|---|
| lewa-góra | PB1 | joystick: X- i Y+ |
| góra | PB2 | joystick: Y+ |
| prawa-góra | PB10 | joystick: X+ i Y+ |
| lewo | PB11 | joystick: X- |
| środek | PB12 | położenie neutralne |
| prawo | PB13 | joystick: X+ |
| lewa-dół | PB14 | joystick: X- i Y- |
| dół | PB15 | joystick: Y- |
| prawa-dół | PC6 | joystick: X+ i Y- |

Anoda każdej diody powinna być podłączona przez rezystor do wybranego
GPIO, a katoda do GND, jeżeli program będzie sterował diodą stanem
wysokim. Przed wykonaniem płytki należy sprawdzić oznaczenie anody
i katody konkretnej diody.

## 5. Konfiguracja SPI

Pierwsza wersja będzie używać **SPI1**:

- `PA5` - `SPI1_SCK`,
- `PA7` - `SPI1_MOSI`,
- tryb master,
- transmisja 8-bitowa,
- dane wysyłane najstarszym bitem jako pierwszym (`MSB first`),
- tryb SPI 0: `CPOL = 0`, `CPHA = 0`,
- sygnał `CS` obsługiwany ręcznie przez PB6,
- sygnały `DC` i `RST` obsługiwane jako GPIO.

Prędkość zegara SPI powinna na początku być zachowawcza, np. 1-4 MHz.
Po poprawnym uruchomieniu wyświetlacza można zwiększyć ją zgodnie
z dokumentacją modułu i jakością połączeń na płytce stykowej.

W projekcie z HAL konfiguracja SPI będzie wygenerowana przez CubeMX, a
sterownik ST7735S otrzyma funkcję transportową opakowującą
`HAL_SPI_Transmit`.

## 6. Organizacja kodu

Kod powinien być podzielony według odpowiedzialności, aby logika gry nie
zależała bezpośrednio od rejestrów mikrokontrolera:

```text
nocka_pwr/
├── Inc/
│   ├── tft_st7735.h       # API wyświetlacza
│   ├── joystick.h         # API odczytu joysticka
│   ├── led_matrix.h       # obsługa wskaźnika LED
│   └── game.h             # stan i reguły gry
├── Src/
│   ├── main.c             # inicjalizacja i główna pętla
│   ├── tft_st7735.c       # komendy ST7735S i rysowanie
│   ├── joystick.c         # ADC, SW i debounce
│   ├── led_matrix.c       # mapowanie kierunku na diody
│   └── game.c             # aktualizacja oraz rysowanie gry
└── README.md              # opis projektu i połączeń
```

Obsługa zdarzeń powinna być oddzielona od sterowników. Przykładowy
przepływ w pętli głównej:

```text
odczyt wejść -> zamiana na zdarzenia -> aktualizacja stanu gry
-> aktualizacja diod -> rysowanie zmian na TFT
```

Przykładowe zdarzenia:

- `JOYSTICK_MOVE`,
- `JOYSTICK_BUTTON_PRESSED`,
- `GAME_TICK`,
- `GAME_RESTART`.

## 7. Pierwsze testy

### Test 1: LED LD2 i B1

Sprawdzić, czy program:

- konfiguruje PA5 jako wyjście,
- konfiguruje PC13 jako wejście,
- reaguje na pojedyncze naciśnięcie,
- nie przełącza stanu wielokrotnie podczas przytrzymania.

### Test 2: zasilanie TFT

Przed podłączeniem sygnałów sprawdzić:

- `VCC` modułu jest połączone z `3V3`,
- `GND` modułu jest połączone z `GND`,
- podświetlenie `BL` działa po podłączeniu do 3V3.

### Test 3: reset i komendy ST7735S

Program powinien:

1. ustawić `CS`, `DC` i `RST` w stan początkowy,
2. wykonać reset sprzętowy przez `RST`,
3. wysłać komendę wyjścia ze stanu uśpienia,
4. ustawić format koloru RGB565,
5. ustawić obszar obrazu,
6. wypełnić ekran jednym kolorem.

### Test 4: joystick

Odczytać wartości `ADC` dla osi X i Y. W położeniu neutralnym wartości
powinny znajdować się w przybliżeniu w środku zakresu. Wartości progowe
należy dobrać do konkretnego modułu, z uwzględnieniem strefy martwej.

### Test 5: wskaźnik 3x3

Poruszać joystickiem w dziewięciu kierunkach i sprawdzić, czy zapala się
właściwa dioda. Strefa środkowa powinna zapalać tylko diodę centralną.

## 8. Ważne uwagi elektryczne

- STM32L476RG pracuje z logiką 3,3 V.
- Nie podawać 5 V na piny GPIO ani wejścia ADC.
- Dla każdej diody LED użyć rezystora szeregowego.
- Wszystkie moduły muszą mieć wspólną masę.
- Przed uruchomieniem sprawdzić omomierzem, czy nie ma zwarcia między
  `3V3` i `GND`.
- Połączenia na płytce stykowej powinny być możliwie krótkie, szczególnie
  dla sygnałów SPI.

## 9. Status projektu

Na obecnym etapie działają podstawowe testy GPIO dla LED LD2 i przycisku
B1. Obsługa TFT, joysticka, diod zewnętrznych i gry jest kolejnym etapem.
Mapa połączeń w tym dokumencie jest planem pierwszego prototypu i powinna
zostać potwierdzona po fizycznym podłączeniu elementów.

