# Projekt gry joystickiem i wskaźnika kierunku

## 1. Co to za projekt

Projekt jest realizowany na **NUCLEO-L476RG** z mikrokontrolerem
**STM32L476RG** i elementami zestawu FORBOT STM32L4. Na tym etapie nie
budujemy pojazdu. Tworzymy demo gry:

- joystick analogowy steruje żółtym punktem na TFT,
- dziewięć diod LED pokazuje kierunek wychylenia joysticka w układzie 3x3,
- przycisk joysticka będzie później zdarzeniem gry,
- TFT Waveshare 1.8" 128x160 ze ST7735S pokazuje obszar gry.

Szczegółowe przewody wyświetlacza są w
[DISPLAY_ST7735S.md](./DISPLAY_ST7735S.md). Ten plik jest punktem odniesienia
przy podłączaniu TFT.

## 2. Poruszanie się po plikach

```text
nocka_pwr/
├── README.md              # cel, pliki i kolejność testów
├── DISPLAY_ST7735S.md     # przewody TFT i konfiguracja SPI2
├── Inc/                   # deklaracje modułów (.h)
│   ├── hw.h               # adresy rejestrów i pomocnicze GPIO
│   ├── tft_st7735.h       # API wyświetlacza
│   ├── joystick.h         # typy kierunku i API joysticka
│   ├── led_matrix.h       # sterowanie diodami 3x3
│   └── game.h             # interfejs logiki gry
└── Src/                   # implementacje modułów (.c)
    ├── main.c             # inicjalizacja i główna pętla
    ├── hw.c               # opóźnienia, GPIO i SystemInit
    ├── tft_st7735.c       # SPI2, reset ST7735S i rysowanie
    ├── joystick.c         # ADC1: VRx/VRy oraz przycisk SW
    ├── led_matrix.c       # mapowanie kierunku na diodę
    └── game.c             # pozycja punktu i ekran gry
```

### Rola plików

- `main.c` uruchamia moduły w kolejności: inicjalizacja, odczyt wejścia,
  aktualizacja LED i aktualizacja gry.
- `joystick.c` zamienia napięcia osi na `JoystickDirection`.
- `led_matrix.c` zapala jedną diodę odpowiadającą kierunkowi.
- `tft_st7735.c` zawiera protokół ST7735S, ale gra korzysta tylko z API.
- `game.c` przesuwa punkt i pilnuje granic ekranu.
- `hw.c` zawiera wspólne operacje niskiego poziomu.

## 3. Plan projektu

1. Podłączyć TFT według [DISPLAY_ST7735S.md](./DISPLAY_ST7735S.md).
2. Podłączyć joystick, dziewięć LED i rezystory.
3. Wykonać test zasilania i ciągłości połączeń.
4. Uruchomić test TFT: reset, inicjalizacja i ramka na ekranie.
5. Uruchomić test odczytu joysticka przez ADC.
6. Uruchomić test wskaźnika LED 3x3.
7. Połączyć wszystko w demo gry.
8. Dodać debounce przycisku `SW` i pierwsze reguły gry.

## 4. Połączenia joysticka i LED

### Joystick

| Pin joysticka | STM32L476RG | Funkcja |
|---|---|---|
| GND | GND | masa |
| +5V / VCC | 3V3 | zasilanie modułu bezpieczne dla ADC |
| VRx | PA0 / A0 | ADC, oś X |
| VRy | PA1 / A1 | ADC, oś Y |
| SW | PB0 | wejście z pull-up, aktywne stanem niskim |

### Wskaźnik LED 3x3

Każda dioda musi mieć własny rezystor szeregowy, np. 330 ohm. Anoda idzie
przez rezystor do GPIO, a katoda do GND.

| Pozycja | GPIO |
|---|---|
| lewa-góra | PB1 |
| góra | PB6 |
| prawa-góra | PB7 |
| lewo | PB8 |
| środek | PB9 |
| prawo | PC6 |
| lewa-dół | PC7 |
| dół | PC8 |
| prawa-dół | PC9 |

Piny `PB10`, `PB11`, `PB12`, `PB2` są zarezerwowane dla sterowania TFT
i nie należy podłączać do nich LED.

## 5. Bezpieczeństwo

- Nie podawać 5 V na VCC TFT, GPIO ani wejścia ADC.
- Każda dioda LED wymaga osobnego rezystora.
- Wszystkie moduły muszą mieć wspólną masę.
- Zasilanie wyłącz przed przepinaniem przewodów.
- Przewody SPI powinny być możliwie krótkie.

## 6. Pierwsze testy w praktyce

Wykonuj testy w tej kolejności. Nie przechodź dalej, jeśli poprzedni test
nie działa.

### Test A — kontrola połączeń

1. Odłącz USB od Nucleo.
2. Podłącz TFT dokładnie według `DISPLAY_ST7735S.md`.
3. Sprawdź wzrokowo, czy `VCC` nie jest zwarte z `GND`.
4. Dołącz joystick i LED-y dopiero po sprawdzeniu TFT.

### Test B — kompilacja i TFT

1. Otwórz projekt `nocka_pwr` w STM32CubeIDE.
2. Wybierz **Project > Build Project**.
3. Uruchom program przez **Debug** lub **Run**.
4. Oczekiwany wynik: ekran ma czarne tło i niebieską ramkę.
5. Jeśli TFT nie reaguje, sprawdź kolejno `VCC`, `GND`, `CLK`, `DIN`,
   `CS`, `DC` i `RST`; nie zmieniaj jeszcze kodu.

### Test C — joystick i LED

1. Ustaw joystick w centrum: powinna świecić środkowa dioda.
2. Wychyl go w osiem kierunków: powinna świecić właściwa dioda.
3. Na TFT żółty punkt powinien przesuwać się zgodnie z kierunkiem.
4. Jeśli osie są zamienione, zamień `VRx` z `VRy` albo mapowanie w
   `joystick.c`.
5. Jeśli kierunek jest odwrotny, odwróć warunki `x_left/x_right` lub
   `y_up/y_down` w `joystick.c`.

### Test D — przycisk joysticka

`SW` jest odczytywany w `JoystickState`, ale jeszcze nie zmienia zasad gry.
Można sprawdzić jego stan w debuggerze. Następny krok to debounce i
zdarzenie `JOYSTICK_BUTTON_PRESSED`.

## 7. Co robić potem

Po pomyślnym przejściu testów:

1. dodać filtrację ADC i kalibrację środka joysticka,
2. dodać debounce przycisku `SW`,
3. oddzielić zdarzenia od bezpośredniego odczytu sprzętu,
4. dodać tekst i ekran startowy TFT,
5. zamienić demo punktu na grę, np. zbieranie punktów,
6. dodać licznik punktów, restart i warunek zwycięstwa,
7. dopiero potem zwiększać częstotliwość SPI i optymalizować rysowanie.

## 8. Status

Kod prototypu znajduje się w `Inc` i `Src`. Kod TFT i dokumentacja są zgodne
z mapą **SPI2: PB10 = CLK, PC3 = DIN** oraz `PB12 = CS`, `PB11 = DC`,
`PB2 = RST`. Przed testem fizycznym wykonaj kompilację w STM32CubeIDE.
