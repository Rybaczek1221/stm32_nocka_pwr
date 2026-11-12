# Czujniki dodatkowe

Dokument opisuje dwa moduły używane lub planowane w projekcie PWR Night Arcade:

- **Crowtail-MPU6050 Accelerometer & Gyro 2.0**, oznaczenie `CRT02122G002`,
- **KAmodVL6180**.

Oba moduły mogą pracować na wspólnej magistrali **I2C**. MPU6050 jest już
obsługiwany przez sterownik `mpu6050.c` i ekran telemetrii. VL6180 pozostaje
przygotowany do późniejszej integracji z grą Paint.

## Wspólne zasady

- używać zasilania zgodnego z opisem na konkretnej płytce,
- połączyć masy STM32, MPU6050 i VL6180,
- linie SDA i SCL muszą mieć podciąganie do napięcia logiki,
- nie łączyć bezpośrednio sygnałów 5 V z GPIO STM32,
- oba moduły mają adresy na I2C, dlatego mogą być podłączone równolegle.

Rekomendowane piny dla STM32L476RG:

| Sygnał | Pin STM32 | Uwagi |
|---|---|---|
| I2C1_SCL | PB8 | zegar magistrali |
| I2C1_SDA | PB9 | dane magistrali |
| GND | GND | wspólna masa |
| VCC | 3V3 albo napięcie zgodne z modułem | sprawdzić opis płytki |

PB8 i PB9 nie są obecnie zajęte przez TFT ani joystick. Przed włączeniem I2C
w CubeMX należy sprawdzić, czy nie są zajęte przez inną funkcję wariantu
płytki.

## Crowtail-MPU6050 Accelerometer & Gyro 2.0

### Połączenie

Moduł Crowtail używa interfejsu I2C. Typowe wyprowadzenia to `VCC`, `GND`,
`SCL` i `SDA`; na złączu Crowtail mogą być opisane jako odpowiednie linie
magistrali. Podłącz:

| Crowtail MPU6050 | STM32 |
|---|---|
| VCC | 3V3, jeżeli wymaga tego wersja modułu |
| GND | GND |
| SCL | PB8 / I2C1_SCL |
| SDA | PB9 / I2C1_SDA |

Typowy adres MPU6050 to `0x68`. Po zmianie stanu pinu AD0 może być użyty
adres `0x69`. Warto potwierdzić adres w dokumentacji konkretnego modułu.

### Możliwości

- pomiar przyspieszenia w osiach X, Y i Z,
- pomiar prędkości kątowej w osiach X, Y i Z,
- wykrywanie przechyłu i gwałtownego ruchu,
- obliczanie prostych gestów, np. potrząśnięcia lub obrotu,
- opcjonalnie temperatura układu.

W projekcie czujnik może służyć do:

- sterowania wybraną grą przechyleniem płytki,
- wykrywania potrząśnięcia jako dodatkowego zdarzenia,
- animacji menu reagującej na ruch urządzenia,
- bonusów aktywowanych obrotem lub gwałtownym ruchem.

### Zalety

- mierzy jednocześnie ruch liniowy i obrót,
- ma dużo zastosowań w grach,
- działa cyfrowo przez I2C,
- pozwala wykrywać gesty niezależnie od joysticka.

### Wady i ograniczenia

- przyspieszenie i żyroskop wymagają filtracji,
- żyroskop dryfuje, więc długie śledzenie kąta bez kalibracji będzie błędne,
- gwałtowne ruchy mogą powodować szum i fałszywe gesty,
- orientacja osi zależy od fizycznego ułożenia modułu,
- trzeba dobrać częstotliwość odczytu i zakres pomiarowy do gry.

## KAmodVL6180

### Połączenie

KAmodVL6180 jest czujnikiem odległości opartym o pomiar czasu przelotu
światła (ToF). Używa I2C:

| KAmodVL6180 | STM32 |
|---|---|
| VCC | 3V3 albo napięcie zgodne z modułem |
| GND | GND |
| SCL | PB8 / I2C1_SCL |
| SDA | PB9 / I2C1_SDA |

Typowy adres czujnika to `0x29`. Adres jest 7-bitowy; biblioteki HAL
często oczekują wartości przesuniętej o jeden bit podczas transmisji.

### Możliwości

- pomiar odległości od przeszkody w krótkim zasięgu,
- pomiar natężenia światła otoczenia,
- wykrywanie zbliżenia dłoni lub obiektu,
- tworzenie dodatkowego sterowania bez dotykania joysticka.

W projekcie czujnik może służyć do:

- aktywowania bonusu po zbliżeniu dłoni,
- wykrywania przeszkody przed urządzeniem,
- sterowania ekranem menu gestem zbliżenia,
- dodania trybu reakcji i szybkości.

### Zalety

- pomiar cyfrowy, bez konieczności kalibracji ADC,
- mały i prosty interfejs I2C,
- pomiar odległości jest wygodny do gestów,
- może dostarczać również pomiar światła.

### Wady i ograniczenia

- krótki zasięg i zależność od powierzchni obiektu,
- czarne, błyszczące lub pochylone obiekty mogą pogarszać pomiar,
- światło zewnętrzne może wpływać na stabilność,
- wynik wymaga obsługi timeoutu i błędów I2C,
- czujnik nie zastępuje dokładnego dalmierza na większą odległość.

## Kolejność uruchamiania

1. Sprawdzić napięcie zasilania i wspólną masę.
2. Podłączyć tylko jeden moduł I2C.
3. Włączyć I2C1 w CubeMX na PB8/PB9.
4. Wykonać skan magistrali i potwierdzić adres `0x68`/`0x69` albo `0x29`.
5. Dodać timeouty transmisji i komunikat błędu.
6. Dopiero potem połączyć oba czujniki i użyć ich w grach.

## Pięć diod życia

W ekranie testowym LED-y są ułożone od lewej jako: **czerwona, czerwona,
żółta, zielona, zielona**. Przy pełnym stanie życia świecą wszystkie, a po
każdej utracie życia gaśnie kolejna dioda od PC6 do PC10.

| Dioda | Pin |
|---|---|
| Życie 1 | PC6 - czerwona |
| Życie 2 | PC7 - czerwona |
| Życie 3 | PC8 - żółta |
| Życie 4 | PC9 - zielona |
| Życie 5 | PC10 - zielona |

Każda dioda musi mieć osobny rezystor szeregowy, np. 220–1 kOhm. Anodę
łączymy przez rezystor z pinem STM32, a katodę z GND. Moduł `lives` gasi
jedną diodę po utracie życia.
Po utracie ostatniego życia wykonuje krótki efekt migania wszystkich diod.

### Montaż na płytce stykowej

1. Wyłącz zasilanie płytki Nucleo.
2. Doprowadź `GND` STM32 do niebieskiej szyny masy na płytce stykowej.
3. Wstaw pięć diod tak, aby ich nóżki były po przeciwnych stronach
   środkowej przerwy płytki. Dłuższa nóżka to anoda, krótsza to katoda.
4. Wszystkie katody połącz z szyną `GND`.
5. Każdą anodę połącz przez osobny rezystor z: `PC6`, `PC7`, `PC8`,
   `PC9` albo `PC10`.
6. Nie łącz anod bezpośrednio z pinami i nie używaj jednego rezystora
   dla kilku diod.

Ułóż diody w jednej linii od czerwonych do zielonych i zachowaj kolejność
z tabeli, bo odpowiada ona kolejności gaszenia.

## Kalibracja MPU6050 do gry

### Pokaz telemetrii

Ekran **CZUJNIKI** działa jako demonstrator na żywo. Opisy i ramka są
rysowane tylko przy wejściu na ekran, natomiast co kilka cykli zmieniają się
wyłącznie pola ACC/GYR, status I2C oraz mały wskaźnik przechyłu. Marker
przesuwa się względem środka, więc można od razu pokazać młodzieży, że
przechylenie płytki zmienia dane i sterowanie. Nie ma już migotania całego
ekranu ani ponownego rysowania menu przy każdym odczycie.

W menu głównym wybierz joystickiem pozycję **CZUJNIKI**, a następnie
**PAINT** albo **BALANS**. Połóż płytkę płasko i nie poruszaj nią przez
krótki moment, aż licznik dojdzie do 12 próbek. Program zapamiętuje średnią
obu osi akcelerometru jako pozycję neutralną.

W grze PAINT przechylenie w obu osiach porusza kropką i zostawia ślad.
Joystick nie steruje kursorem. Sterownik filtruje pomiary i stosuje strefę
martwą, więc płaska płytka utrzymuje kropkę w miejscu. Szybki obrót płytki
zmienia kolor pędzla.

Gra **BALANS** wykorzystuje ten sam skalibrowany odczyt: zielony cel zmienia
położenie, a punkty są naliczane za utrzymanie kropki w celu. To tryb
demonstracyjny oparty wyłącznie na MPU6050.
