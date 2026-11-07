# BloomML — IPO Quality Gate B5 (2026-11-07)

## Zakres sprintu (source only)

**Stan przed pracą:** `main` f41dd8264e8c34ce50f8aa520b4d86153e33b7d4. Przeprowadzono preflight GitHub `main` i `agent-control` (idle). Prace tylko na `main`, bez Local Agent / GitHub Actions / Codex / Work.

### 1. Bezpieczne dopuszczanie profilu IPO do bieżącego CrowPanel

- `lib/ipo_profile/src/IpoBoardAdmission.h`: obowiązkowe aktywne wejście **SCD41 CO₂**, obok TP357 T/RH i RTC. Sam proces `co2_safety` oraz link do wyciągu **nie zastępuje** obecności źródła sygnału. Referencyjna mapa nadal wymaga pięciu istotnych powiązań i trzech właściwych gniazd RF433, czasów ON/OFF oraz zgodnych driverów. `apply_authorized` pozostaje trwale false.
- `lib/ipo_profile/src/IpoSignalSelector.h`: jawny limit liczby pomiarów (maksimum bounded 64) i ochrona przed konwersją skończonego `double` przekraczającego zakres `float`. Invalid/stale pozostaje invalid; nie ma zera zastępczego.
- `src/climate/runtime/profile/IpoProfileWire.h`: fail-closed odrzucanie kandydatów z `exhaust_fan_available=false`. Obecny system nie ma alternatywnej kwalifikowanej drogi ochronnego wietrzenia.
- `src/climate/runtime/profile/IpoProfileJsonDecode.h`: limit urządzeniowego JSON podniesiony z 8 KiB do **32 KiB** (ograniczony, wciąż nieaktywny parser). Przy 24 urządzeniach + źródłach + linkach poprzedni limit był sprzeczny ze schematem v2. Parsowanie musi działać poza 1-s hot path; przed aktywacją wymaga potwierdzenia realnego heap/PSRAM.

### 2. Jeden przejrzysty status zamiast fałszywego „aktywowano”

- `src/climate/runtime/profile/IpoProfileReadback.h`: observer-only projection `active RAM` vs `stored record`, oddzielnie generacja/hash RAM, zapis w journal, dobra kopia zapasowa, record fingerprint, bez udawania fizycznego apply.
- `src/climate/runtime/profile/IpoProfileDiagnosticNames.h`: stabilne tekstowe nazwy stanów NVS i źródeł sygnałów, zamiast odczytu wyłącznie numerów enum.
- `IpoProfileNvsJournal.h/.cpp`: obliczenie niewymagającego pełnej reenkodacji **CRC32 całego zapisanego rekordu** podczas odczytu NVS (dla kopii aktywnej albo poprawnej zapasowej); cache w `IpoJournalReadResult`. `IpoProfileReadback` nie koduje już grafu 1536 B na stosie obsługi statusu.
- `Stage28ServiceConsoleSystemCommands.cpp`: dodatkowe pola `ipo_health` (fresh/stale/missing + store status) i `ipo_readback` (Mint w RAM, zapis w NVS, generacje, CRC32, backup). Istniejące wiersze diagnostyczne pozostawione dla kompatybilności parserów. Ani konsola, ani NVS nie zyskują uprawnień do wyjść.

### 3. Panel IPO v2 — kompatybilność obecnego firmware, bez blokowania otwartości

- `web/src/domain/ipo-board-readiness.ts`: jeden preflight **tylko informacyjny** dla obecnej mapy CrowPanel — driver/locator, CO₂, role źródeł, 3 mapowania RF433, min czasy ON/OFF, procesy i obowiązkowe linki. Poprawny uniwersalny graf IPO może być **niezgodny z tym konkretnym firmware**. `applyEnabled: false` zawsze.
- Panel `web/src/pages/ipo-setup-page.tsx`: przyczyny niezgodności, wielkość JSON w rzeczywistych bajtach UTF-8, wyraźny brak fizycznego apply. Eksport zminimalizowanego JSON **bez dodatkowego LF**, zgodny z 32 KiB parserem.
- Limit **80 bajtów UTF-8** etykiety w aktualnym parserze C++ jest sprawdzany w preflight (Unicode może mieć mniej znaków niż bajtów). Schemat i otwarty edytor v2 pozostają elastyczne; ograniczenia dotyczą wyłącznie obecnej płytki.

### 4. Celowane kontrakty testowe (zapisane, nieuruchomione)

- `test/test_ipo_profile/test_main.cpp`: referencyjny graf, usunięcie wejścia CO₂, usunięcie safety link, minima dwell, duplicate endpoint, niekwalifikowany driver/fallback, świeżość + granica 30s, NaN/overflow double→float, MAX preview bez bypass safety, schedule, CRC/version/length, typed wire roundtrip, stored vs active readback oraz odmowa dla `exhaust_fan_available=false`.
- `web/src/domain/ipo-board-readiness.test.ts`: podstawowy Mint, brak CO₂, brak link safety, przyszły driver, niekwalifikowany SCD41 T fallback, minima dwell, podwójny endpoint, UTF-8 i pomiar bajtów.

## Zakres NIE wykonany

- **Nie uruchamiano testów, lint, typecheck, ESP-IDF build ani flashowania** — zgodnie z wcześniejszym życzeniem unikania ponawiania i ciężkich buildów. Nowy kod i testy są `UNVERIFIED`; samo dodanie testów nie dowodzi ich przejścia.
- W szczególności brak wąskiej kwalifikacji `CrowPanel` exact-SHA; źródła i parsowanie mogą zmieniać zachowanie Rule, dopóki nie zostaną sprawdzone.
- Brak NVS `commit` w aktywnym runtime, brak aktywnego DeviceProfile v2, brak rollbacku **całego runtime** i sterowania dynamicznymi wyjściami. Szkic w panelu nie zmienia sprzętu.
- Nie dotykano fizycznych ról i `OutputSupervisor`, LampSafety, bieżącego Rule, ML Shadow ani display ownership.

## Jedyny następny krok

**Wąska kwalifikacja nowej ścieżki** (nie historyczny pełny audit) z dedykowanymi testami `test/test_ipo_profile`, `web/...readiness.test.ts`, parserem C++/JSON i sprzętowym profilem build; sprawdzić też zachowanie przy awarii NVS i hardware input stale. Dopiero po PASS opracować jeden właściciel transakcji `stage → validate → commit → atomic apply → active readback → rollback`, a następnie lokalny odczyt statusu w istniejącym panelu. Nie uruchamiać żadnego procesu w tle, nie przypisywać AI/ML władzy wyjściowej.
