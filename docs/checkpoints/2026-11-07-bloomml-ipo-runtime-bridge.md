# BloomML — autonomiczny sprint IPO Runtime Bridge 1 (2026-11-07)

## Zakres kodu zrobiony na main

1. Typowany, walidowany `RuntimeScheduleProfile` w `src/climate/runtime/schedule/RuntimeScheduleProfile.h`: cele dzień/noc, RH/VPD, timeout, dostępność wyciągu/nawilżacza, harmonogram także przechodzący przez północ. Domyślne wartości odpowiadają dokładnie dotychczasowemu Mint. Kontroler `RuntimeScheduleProfileOwner` trzyma zatwierdzony snapshot i jego licznik generacji tylko **w RAM**, bez NVS.
2. `ScheduleProfile.cpp` i `ScheduleIntentAdapter.cpp`: zachowane istniejące funkcje Mint, dodane overloady dla walidowanego snapshotu.
3. `RuntimeAdapters` + `ClimateV6RealInputRuntime.cpp` + `RealInputRuntimeCoordinator`: **real-input path** otrzymuje wspólnego właściciela profilu. Zarówno harmonogram lampy, jak i snapshot wejścia do Rule czytają tę samą konfigurację. Nie ma zewnętrznego `apply` ani zmiany fizycznych endpointów; domyślny stan nie zmienia bieżącego zachowania.
4. `Stage28ServiceConsoleSystemCommands.cpp`: `status` pokazuje wersję/proweniencję snapshotu, generację, nastawy dzień/noc, tryb RH/VPD oraz czy zapis NVS/v2 jest używany. Aktualnie: built-in Mint, `persisted=0`, `profile_v2_applied=0`.
5. `lib/ipo_profile/src/IpoBoardAdmission.h` + `IpoSignalSelector.h`: bounded, header-only mechanizmy rozpoznania zgodności aktualnego CrowPanel i wyboru świeżego, priorytetowego źródła semantycznego. Admission **odrzuca** niezmapowane drivery, niezgodne źródła, wyjścia i obniżenie min. czasów. Przykładowy referencyjny graf Mint jest tylko informacyjny. Selektor NIE zastępuje jeszcze podłączonych fizycznych źródeł w runtime.
6. `src/CMakeLists.txt` i startup: dołączony przenośny katalog IPO jako nagłówki; przy starcie raportowana zgodność referencyjnego grafu i liczby urządzeń/sygnałów/wyjść. `can_apply=0` niezależnie od zgodności.

## Niezmienione i czego nie wolno twierdzić

- **OutputSupervisor** nadal jako jedyny właściciel, stałe RF433 fan/lamp/humidifier, lamp safety >=28C i odzyskanie <=26C przez 10 min, Rule autorytatywny, Shadow pasywny, brak nowych komend sprzętowych, brak flashowania.
- Nie dodano NVS dla całego IPO v2, nowego parsera profilu do ESP, zewnętrznego apply, readback `active hash`, dynamicznego fallbacku SCD41 T/RH, ani integracji przeglądarki z urządzeniem. Odczyt statusu to **tylko pamięć RAM wbudowanego Mint**, nie dowód aplikacji v2.
- Brak nowych testów/kompilacji zgodnie z życzeniem operatora. Zmieniony kod C++ i jego integracja mają **UNVERIFIED**; nie deklarować działania na sprzęcie ani wdrożenia.

## Jedyny następny etap

1. Związać strict v2 JSON z bounded DTO i z board admission, bez możliwości ominięcia odmowy.
2. Zaprojektować i wdrożyć osobną domenę NVS dla **całego** profilu (nie tylko targetów) z integralnością, atomowym active/pending/known-good i rollbackiem; nie dotykać NVS OutputSupervisor.
3. Użyć już istniejącego selektora sygnałów za rzeczywistymi driverami i narzucić `valid/age` oraz kwalifikowany fallback (zwłaszcza SCD41 T/RH) bez fałszywego zera.
4. Dopiero po rozdzielnym dowodzie przyjąć dynamiczne mapowanie wyjść do istniejącego OutputSupervisor i dopuścić `Preview → Apply → Readback` z faktycznym potwierdzeniem, podgląd statusu także na e-ink.

Start od świeżego main; brak nowych branches, Codex, Work, GitHub Actions. Nie powtarzać starszych testów; nowe zmiany przed realnym sterowaniem wymagają osobnej wąskiej kwalifikacji.

## Stage 2 source extension: full typed profile persistence journal (same sprint)

- `src/climate/runtime/profile/IpoProfileWire.h`: explicit little-endian wire encoding/decoding of **whole bounded IPO graph plus day/night/timeout/capabilities and generation**, not a padded raw struct and not research JSON. Version, magic, CRC32, maximum payload 1536 bytes and board admission before accepting. Decode refuses length/CRC/unsupported hardware mismatch. Exporting browser JSON still needs a separate trusted parser/normalizer.
- `src/climate/runtime/profile/IpoProfileNvsJournal.h/.cpp`: separate NVS namespace `gb_ipoprof`; two durable slots, inactive slot write+commit, exact byte readback, CRC and typed decode, then selector switch+commit+readback. Boot-side `load` detects absent/corrupt slots and reports a valid previous slot for operator-approved recovery. Stale generation is rejected. **Does not call global NVS init/erase**, does not touch OutputSupervisor state store.
- Added journal C++ file to real-input CMake source list, but **no instance is called during runtime boot or execution**. No automatic read/apply or write is enabled, so currently deployed behavior does not change.
- `IpoGraph.h` guards maximum four source priorities; selector iterates only those priorities.
- Validation and physical qualification of the new codec/NVS journal remain **NOT DONE** by operator request; do not claim it is production-proven, crash-safe under all torn writes, or active on the board. Serialize future store calls through a single runtime owner: generation checking alone is not a cross-task transaction mutex.

Now next milestone is full v2 JSON parse/normalize → capability/admission → journal transaction → runtime atomic apply with rollback → profile hash/readback and live status. Do **not** wire journaled candidate to physical outputs before narrow qualification.
