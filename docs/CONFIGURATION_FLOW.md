# BloomML — kanoniczna mapa IPO v2

**2026-11-07 · aktualny kierunek: IPO Foundation v2.** Ta strona jest jedyną główną mapą produktu i jego stanu. Szkic v2 NIE jest jeszcze zastosowany na ESP32-S3.

## Przepływ

```text
Konfigurator /
  1. DeviceRegistry: instancje sprzętu, drivery, adresy
  2. SignalRegistry: sygnały semantyczne + główne/rezerwowe źródła
  3. ProcessRegistry: typowane procesy, cele dzień/noc, harmonogram
  4. OutputRegistry: urządzenia wykonawcze, kanały, role, minima ON/OFF
  5. ProcessBindings: proces --dozwolony efekt--> wyjście
  6. Walidacja i import/eksport DeviceProfile v2
                           |
             [BRAK RUNTIME APPLY v2]
                           v
  Docelowy firmware: validate → NVS → atomowy apply → readback
  wejścia → Rule / ML Shadow(propozycja tylko) → arbiter
          → Safety → OutputSupervisor → transport → telemetria
                           |
                   prawdziwe trajektorie
                           v
            kalibracja symulatora → trening → Shadow
```

## Istniejące źródła i urządzenia, nie nowe uprawnienia

| Co | Aktualne źródło/endpoint | Rola |
| --- | --- | --- |
| T/RH wewnątrz | TP357 BLE | Rule T/RH/VPD i zabezpieczenie lampy |
| CO₂ | SCD41 I²C | CO₂; T/RH z SCD41 NIE są jeszcze fallbackiem runtime |
| T/RH otoczenia | Xiaomi BLE | informacja do sterowania wyciągiem |
| Czas | DS3231 | sztywny Mint dzień/noc |
| Wyciąg | RF433 socket:1 | Rule + safety, binary dwell 120/120 s |
| Lampa | RF433 socket:2 | harmonogram + termiczne safety; nie ML |
| Nawilżacz | RF433 socket:3 | Rule + safety, binary dwell 180/180 s |
| E-ink | SSD1680 | obserwator, bez własności sterowania |

Pozostałe role (grzałka, chłodzenie, osuszacz, dozownik CO₂, pompa, GPIO/PWM/Modbus) są w nowym profilu możliwościami *projektowymi*, ale NIE fizycznie działającymi wyjściami obecnego firmware. RF433 nie daje samoistnego potwierdzenia stanu gniazda.

Aktualne sztywne nastawy Mint: dzień 06–22: 24.5°C / 58% RH / 1.2 kPa; noc: 21.5°C / 65% RH / 0.9 kPa; tryb RH; CO₂ target off, timeout 30 s. Termiczny interlock lampy: wyłączenie od ≥28°C, powrót dopiero po ≤26°C przez 10 minut. Parametry te są nadal w firmware, nie wczytywane z v2.

## Co dostarcza v2

- Schemat: **schemas/device-profile.v2.schema.json**, przykład: **profiles/crowpanel-ipo.v2.json**. Wersja oraz status draft_not_applied są wymagane.
- **DeviceRegistry** — wiele instancji z ID, driverem, adresem i flagą enabled; katalog capability i kanałów w **web/src/domain/ipo-catalog.ts**. Odróżnia driver obecny w firmware od planowanego; dynamiczna rejestracja nawet istniejącego drivera pozostaje otwarta.
- **SignalRegistry** — role niezależne od modelu czujnika, kolejność źródeł, walidacja kanału i jednostki; źródło z nieprawidłowym/starym pomiarem nie może działać jako primary; fizyczny fallback jeszcze niezaimplementowany.
- **ProcessRegistry** — typowane procesy (temperatura, RH/VPD, światło, ochrona CO₂); następne typy mogą być dodawane przez katalog. Chronione safety CO₂ nie może zostać wyłączone w profilu, przy czym fizyczne zabezpieczenia działają niezależnie od grafu.
- **OutputRegistry** — rola logiczna niezależna od kanału; unikatowe fizyczne wyjścia i bezpieczny stan OFF; np. dwa wyciągi na dwóch osobnych kanałach, wiele procesów na jednym wyciągu.
- **ProcessBindings** — dozwolone typowane efekty (np. ventilate dla wyciągu), zakaz przypisywania niedozwolonego działania do urządzenia.
- **OutputArbiter** — wyłącznie symboliczna agregacja zgodnych propozycji przez MAX; bez fizycznych komend. Safety oraz OutputSupervisor pozostają nadrzędne.

Dodatkowo: **lib/ipo_profile/src/IpoGraph.h** zawiera przenośny C++17, ograniczony rozmiarem graf i walidator oraz podgląd agregacji; nie jest podłączony do produkcyjnego ESP-IDF.

Limity szkicu: 24 urządzenia, 16 ról sygnałów, 24 wyjścia, 16 procesów, 48 połączeń oraz 4 źródła na jedną rolę (UI edytuje primary i backup). To są limity rozmiaru danych, nie deklaracja gwarantowanej pojemności hardware.

## UI i zgodność wsteczna

- **/** — nowy konfigurator IPO v2, jedna główna strona, dodawanie i usuwanie urządzeń, źródeł, wyjść, procesów, linków, cele i import/eksport JSON.
- **/lab/profile-v1** — zachowany wcześniejszy sztywny szkic v1. Import v1 do v2 jest jawną migracją walidowanych danych.
- **/lab/v5** — zachowany historyczny research schema v5 (228 features/25 outputs).
- **/chamber-3d** — niezależna geometria, nie rzeczywista fizyka klimatu.
- ML inference climate-v6 (44 features/6 outputs) i symulator badawczy v4 są innymi kontraktami niż DeviceProfile v2.

## Co dokładnie pozostaje do fizycznej integracji

1. Firmware adapter: bezpieczne przetłumaczenie JSON v2 na typowane ograniczone struktury, registry driverów/zasobów sprzętu, weryfikacja dostępności i odrzucanie niezaimplementowanych driverów.
2. Input runtime: dynamiczne role, age/valid/freshness, primary/fallback, brak zamiany brakujących danych na zero.
3. Processing runtime: typowane procesy, zgodne intencje, harmonogram, priorytety, nieusuwalne bezpieczeństwo, shadow pasywny.
4. Output runtime: pojedynczy OutputSupervisor jako właściciel fizycznych wyjść; deduplikacja i ograniczenia endpointów oraz transport z osobną obserwacją stanu.
5. Osobna domena NVS: wersjonowanie, active/pending/known-good, integralność, atomic apply, readback, restart/rollback; awaria nie może częściowo zastosować grafu.
6. Panel + e-ink status: aktywny hash profilu, pomiary i ich wiek, role/źródła, decyzje i stany requested/resolved/attempted/observed. UI draft nie może udawać runtime.
7. Symulator: wirtualne adaptery tych samych ról v2 oraz kalibracja na rzeczywistych danych. Brak automatycznej aktywacji MPC.

## Uczciwe bramki

Zmieniono tylko izolowane DTO/katalog/walidację i przeglądarkowy profil. **Nie zmieniono fizycznego Rule, Safety, OutputSupervisor, NVS, RF433 ani e-ink.** Nie wykonano testów ani ciężkich kompilacji zgodnie z życzeniem operatora. Nowy kod ma status niezweryfikowanego szkicu integracyjnego.

## IPO Runtime Bridge 1 — wpięcie w C++ firmware (2026-11-07)

Prócz samego szkicu v2 działa teraz w kodzie **wspólny typowany właściciel profilu RAM** `RuntimeScheduleProfileOwner`, odczytywany zarówno przez `ConfiguredScheduleConfigSource` (Rule), jak i `ScheduleIntentAdapter` (harmonogram lampy). Nadal stosuje tylko wbudowany Mint; brak wejścia z JSON, NVS, fizycznego apply. Stare entrypointy Mint zachowano. Nie udawać, że dwa czytania zegara w jednym cyklu są gwarantowanie jednym identycznym pomiarem czasu — wspólna jest konfiguracja, a dedykowany clock snapshot pozostaje dalszą integracją.

Nowe `lib/ipo_profile/src/IpoBoardAdmission.h` ma fabrykę **referencyjnego** grafu aktualnego CrowPanel oraz ścisłą bramkę obecnych driverów/wejść i RF433. `IpoSignalSelector.h` implementuje niezależny wybór świeżego źródła (valid/age/priority), **bez** podłączenia go do produkcyjnych sensorów. Real firmware otrzymał tylko **diagnostyczny** odczyt zgodności grafu przy boot (`can_apply=0`). Komenda konsoli `status` pokazuje snapshot w RAM: generację i bieżące cele/harmonogram, jawnie `persisted=0`, `profile_v2_applied=0`. `OutputSupervisor` i fizyczne RF433 pozostają niezmienione.

**Nie uruchamiano testów ani kompilacji** zgodnie z wyraźną dyspozycją. Kod po tych zmianach jest UNVERIFIED; na urządzeniu jeszcze nie wdrożony. Szczegóły i kolejność następnych etapów: [checkpoint](checkpoints/2026-11-07-bloomml-ipo-runtime-bridge.md), [handoff](NEXT_CHAT.md).

## IPO Runtime Bridge 2 — źródłowy magazyn całego profilu (2026-11-07)

Dodano `src/climate/runtime/profile/IpoProfileWire.h` (typowany, wersjonowany, little-endian format całego grafu IPO i nastaw, CRC32, bounded 1536 B) oraz `IpoProfileNvsJournal.h/.cpp` (oddzielna przestrzeń NVS, dwie kopie, nieaktywny slot → weryfikacja → przełączenie aktywnego znacznika → readback, wykrywanie uszkodzenia i raport poprzedniej dobrej kopii). Ta warstwa odmówi zapisania profilu z unsupported driver/mapping na tym konkretnym CrowPanel. Jest to **kod magazynu przygotowany i dodany do listy źródeł**, lecz obecnie NIKT go nie wywołuje w boot ani w panelu; profile v2 nie są zapisywane, odczytywane ani stosowane do fizycznego sterowania. Cross-task synchronizacja musi być zapewniona przez przyszłego pojedynczego właściciela apply; nie ma jeszcze atomowego przełączenia konfiguracji runtime. Nie wykonywano testów/kompilacji.

## IPO Runtime Bridge 3 — firmware parser, bez wykonania (2026-11-07)

`IpoProfileJsonDecode.cpp` mapuje `DeviceProfile v2` JSON na bounded `Graph + RuntimeScheduleProfile` i **odrzuca** nieznane pola, niezgodne identyfikatory/kanały, drivery bez adaptacji sprzętowej oraz mapowania poza CrowPanel. `IpoProfileStagingCoordinator.h` tworzy izolowaną ścieżkę `preview → typed admission → zapis-tylko-do-NVS → readback`. Żaden komponent aktywnego bootstrapu jej nie wywołuje; nie ma nadawania uprawnień RF/OutputSupervisor i nie ma jeszcze aktualizacji aktywnego profilu z NVS. Kontrakt binary zapisuje semantyczną konfigurację wykonawczą, **nie** pełne stringowe etykiety JSON. `status` raportuje `schedule_hash` obecnego Mint RAM, a nie aktywny hash grafu v2. Bez testów i kompilacji: UNVERIFIED.

**Dodatkowa bramka zgodności:** poprawne numery gniazdek nie wystarczą; adapter odrzuca graf bez kompletu pięciu wymaganych połączeń proces→efekt→wyjście znanych dla bieżącego Mint. Dalsze rozszerzanie ról wyjść wymaga nowych driverów i kwalifikacji; nie powoduje automatycznego odblokowania RF.


## Bridge 4 — selection of real inputs + read-only NVS boot inspection (2026-11-07)

W real-input runtime źródła `RuntimeInsideSource` i `RuntimeNearbySource` teraz używają **semantycznego selektora IPO** dla istniejącego, referencyjnego grafu CrowPanel. Identyfikatory: 1=TP357 (T/RH wewnątrz), 2=SCD41 (tylko CO₂), 3=Xiaomi (T/RH zewnętrzne). Odrzuca niefinitywne, invalid i starsze niż timeout profilu odczyty. Lamp safety korzysta nadal z niezależnego TP357 i nie przełącza się na SCD41. To jest faktyczne wpięcie selektora, ale nadal NIE jest dynamiczną rejestracją driverów z JSON v2.

Na boot dodano **wyłącznie odczyt** osobnego IPO NVS journal. Nawet gdy znajduje poprawne zapisane bajty, nie podłącza ich do Rule, harmonogramu ani OutputSupervisor. Istniejąca konsola `status` pokazuje `ipo_store` (stan pamięci i generację, `applied=0`) osobno od `ipo_profile` (aktualne cele Mint w RAM) i `ipo_input` (ważność, wiek, źródło ostatnich pomiarów). Żadne urządzenie wykonawcze nie otrzymuje komend z tego magazynu. Nadal nie ma urządzeniowego API dla przeglądarki ani potwierdzenia fizycznego stanu RF433.

**Uwaga dot. bezpieczeństwa:** warunkowanie świeżością pomiaru może zmieniać zachowanie Rule. Zmian nie kompilowano, nie testowano i nie wgrywano na ESP32 na życzenie operatora. Przed dopuszczeniem fizycznej aktywacji wymagana jest dedykowana, wąska kwalifikacja tej nowej ścieżki, a nie ponawianie historycznych testów badawczych.
