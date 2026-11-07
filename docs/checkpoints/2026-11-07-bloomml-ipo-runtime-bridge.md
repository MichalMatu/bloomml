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
