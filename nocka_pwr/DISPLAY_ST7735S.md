# Wyświetlacz TFT ST7735S

Dokument opisuje podłączenie wyświetlacza używanego w 11. części kursu STM32L4 firmy Forbot oraz plan jego obsługi w projekcie `nocka_pwr`.

## Moduł

- moduł Waveshare TFT LCD 1,8"
- sterownik: **ST7735S**
- rozdzielczość: **128 × 160 pikseli**
- interfejs: **SPI, tylko transmisja do wyświetlacza**
- zasilanie: **3,3 V**

Wyświetlacz nie udostępnia linii MISO, więc mikrokontroler nie może odczytywać z niego danych. Linia `BL` ma pozostać niepodłączona; moduł ma własne podciągnięcie podświetlenia.

## Połączenia

| Wyświetlacz | STM32L476RG / NUCLEO-L476RG | Rola |
|---|---|---|
| `VCC` | `3V3` | zasilanie 3,3 V |
| `GND` | `GND` | masa |
| `DIN` | `PC3` | SPI2 MOSI, dane do wyświetlacza |
| `CLK` | `PB10` | SPI2 SCK, zegar |
| `CS` | `PB12` | ręcznie sterowany wybór układu |
| `DC` | `PB11` | dane (`1`) albo komenda (`0`) |
| `RST` | `PB2` | reset sterownika |
| `BL` | nie podłączać | podświetlenie włączone przez moduł |

Przed podłączeniem należy wyłączyć zasilanie płytki. Nie podawać 5 V na `VCC` ani na linie sygnałowe.

## Konfiguracja SPI2

Planowana konfiguracja rejestrów:

- SPI2 jako **Master, Transmit Only**
- format Motorola
- rozmiar ramki: 8 bitów
- kolejność bitów: MSB first
- preskaler: `8`
- NSSP: wyłączone
- przewidywana częstotliwość transmisji: około `10 MHz` przy zegarze SPI2 równym `80 MHz`
- `CS` obsługiwany programowo przez `PB12`

## Plan biblioteki

Biblioteka będzie utrzymywana jako osobny, niewielki moduł bez HAL-a i bez CubeMX:

```text
Src/
    lcd.c
    lcd.h
    spi.c
    spi.h
```

Zakładane warstwy:

1. `spi.c` — zegar SPI2, konfiguracja `PB10`/`PC3` w trybie alternatywnym oraz wysyłanie bajtu.
2. `lcd.c` — sterowanie `CS`, `DC`, `RST`, inicjalizacja ST7735S i ustawianie obszaru obrazu.
3. Funkcje graficzne — najpierw czyszczenie kolorem i pojedynczy piksel, później prostokąty, tekst i ewentualna biblioteka graficzna.
4. `main.c` — tylko inicjalizacja i użycie publicznego API, bez szczegółów protokołu ST7735S.

Pierwsze planowane API:

```c
void lcd_init(void);
void lcd_fill(uint16_t color);
void lcd_draw_pixel(uint8_t x, uint8_t y, uint16_t color);
```

Kolor będzie przesyłany w formacie RGB565. Definicje rejestrów i stałe pinów powinny trafić do odpowiednich plików modułu, a nie pozostać w `main.c`.

## Kolejność implementacji

1. Dodać definicje rejestrów SPI2 i GPIO dla pinów wyświetlacza.
2. Skonfigurować SPI2 oraz sprawdzić wysłanie pojedynczego bajtu.
3. Dodać reset sprzętowy i sekwencję inicjalizacyjną ST7735S.
4. Ustawić orientację oraz zakres `128 × 160`.
5. Wysłać jednolity kolor na cały ekran.
6. Dodać funkcje rysowania i dopiero potem przenieść obsługę do głównej aplikacji.

## Źródła

- Kurs Forbot STM32L4, część 11: kolorowy wyświetlacz TFT (SPI)
- dokumentacja modułu Waveshare 1.8inch LCD Module
- dokumentacja sterownika ST7735S
