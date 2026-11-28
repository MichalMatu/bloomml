# BloomML — aktualny stan

**2026-11-28 · źródło: `MichalMatu/bloomml` / `main`**

**STATUS: H0 PASS / H1 PARTIAL / IPO ACTIVATION CLOSED.**

CrowPanel: autoryzowany port `/dev/cu.usbserial-130`, fizyczna izolacja odbiorników potwierdzona przez operatora. Firmware `2199a75` fake-locked: sprzętowy PASS w zakresie zasobów, normalnych odczytów i zapisu awaryjnego. Native NVS fault/recovery: **SKIPPED — TOOL BLOCK; operator-owned local follow-up**. Nie jest to PASS. H1 pozostaje otwarty; H2/H3 nie są dopuszczone. [Dowody](checkpoints/2026-11-27-bloomml-hardware-progress.md).

Aktualny sprint software M01–M05: **CLOSED_SCOPED**. M06 — integracja software MVP, jeszcze nierozpoczęta. Pozostała kwalifikacja sprzętowa odroczona na decyzję operatora; nie ponawiać zakończonych ani zablokowanych prób. [Punkt wznowienia](NEXT_CHAT.md).

## Najnowszy sprint software — bez aktywacji IPO

M05: autonomiczny Rule wieloparametrowy — software CLOSED_SCOPED; [checkpoint M05](checkpoints/2026-11-28-bloomml-m05-multiparameter-rule.md). Host 62/62, web 117/117, Python 13, architektura 3/3 i CrowPanel build PASS. Bez zmiany kwalifikacji sprzętowej.

M03: konfigurowalne wejścia IPO — software CLOSED_SCOPED; [checkpoint M03](checkpoints/2026-11-28-bloomml-m03-configurable-inputs.md). Host, web i CrowPanel build PASS na kodzie 4fc585d; alternatywne profile pozostają nieaktywne sprzętowo.

M04: konfigurowalne wyjścia IPO — software CLOSED_SCOPED; [checkpoint M04](checkpoints/2026-11-28-bloomml-m04-configurable-outputs.md). Host 62/62, web 116/116, architektura 3/3, CrowPanel build PASS. Produkcja nadal wyłącznie Mint; nowe role i adaptery tylko software/fake, zero wyjść = jawna odmowa uruchomienia. H2/H3 zamknięte.

M02: separacja portów I/P/O — software CLOSED_SCOPED; szczegóły i dowody w [checkpoint M02](checkpoints/2026-11-28-bloomml-m02-ipo-separation.md). Mint pozostaje konfiguracją referencyjną, fizyczna aktywacja IPO nadal zamknięta.

Naprawy `cf1c434` (web typecheck, HTTP diagnostics POST), `196c305` (96 naruszeń lint przeniesionych do wspólnego `app-chrome`) i `4aadf5f` (świeżość odczytu, timeout 504) zakończone. Web na `196c305`: **114/114 Vitest PASS**, typecheck/lint/build PASS; panel na `4aadf5f`: **135/135 pytest PASS**; host C++/display na `cf1c434`: **PASS**; bazowy IPO CTest na `54c486d`: **3/3 PASS**. Są to wyniki software na Macu, nie fizyczne testy CrowPanel. [Pełny checkpoint](checkpoints/2026-11-07-bloomml-general-repository-repair.md).

## Co naprawdę działa w źródłach

| Warstwa | Stan i granica |
| --- | --- |
| Sterowanie | Deterministyczny **Rule**; ML/MPC wyłącznie Shadow/research |
| Wyjścia | Jedyny normalny właściciel: **OutputSupervisor**; RF433 1 wyciąg, 2 lampa, 3 nawilżacz |
| Bezpieczeństwo | Niezależny TP357 LampSafety: trip ≥28°C; recovery ≤26°C nieprzerwanie przez 10 minut |
| Operator | `/` to offline szkic DeviceProfile v2; walidacja grafu, board readiness, import/eksport; **brak apply do sprzętu** |
| Wejścia | Wspólny rejestr odczytów BLE T/RH i SCD41 T/RH/CO₂, role i priorytety z grafu; DS3231 przez konfigurowalny zegar. Mint na starcie bez zmian; brak dynamicznej rejestracji driverów i aktywacji innych grafów |
| Harmonogram | Jeden `RuntimeScheduleProfileOwner` w RAM dla Rule i lampy; na starcie **wbudowany Mint** |
| NVS IPO | Dwa sloty `gb_ipoprof`, CRC, generation, rollback; boot tylko **odczytuje stan** do diagnostyki |
| Transakcja | `IpoProfileActivationTransaction`: preview → validate → NVS commit → RAM apply → readback/rollback. Produkcyjny `ClosedIpoActivationGate` **zawsze odmawia**; brak wywołania z UI/UART |
| Odczyt statusu | Rozdzielone `ipo_store` / `ipo_profile` / `ipo_readback` / `ipo_activation`. Zapisany NVS ≠ aktywny Mint ≠ fizyczny stan RF |
| E-ink | Native SSD1680 + Clay, jeden framebuffer, asynchroniczny obserwator; przyciski wymagają odrębnej kwalifikacji fizycznej |

Źródło konfiguracji i granice danych: [CONFIGURATION_FLOW.md](CONFIGURATION_FLOW.md); architektura: [ARCHITECTURE.md](ARCHITECTURE.md).

## Zaliczone bramki — nie powtarzać bez zmiany dotyczącego ich kodu

| Test | Wykonanie | Wynik |
| --- | --- | --- |
| Real-input CrowPanel N8R8 + e-ink ESP-IDF build | Mac Local Agent, kod `1a1b3de8` | **PASS**, bin `0xd1e00`; 80% partycji aplikacji wolne |
| Host IPO parser/activation/NVS journal | `make test-ipo`, kod `45f3527` | **3/3 PASS** |
| Produkcyjny `IpoProfileNvsJournal.cpp` z mock NVS fault injection | kod `347bcca` | **1/1 PASS** (zawarte w 3/3) |
| LampSafety, BLE stale → Rule, output lifecycle | kod `347bcca` | **3/3 PASS** |
| IPO board readiness Vitest | wcześniejszy kod `977c48d` | **9/9 PASS** |
| Końcowy przegląd dokumentów i closed gate poprzedniego etapu | kod `c4bf62cf` | **PASS**, czyste repo |

*Ważne:* NVS fault injection używa podmienionego API, **nie fizycznego flash**. Kompilacja ESP-IDF nie dowodzi zachowania czujników ani poprawnego działania na podłączonej płytce. SHA testów i builda są różne. Późniejsze zmiany produkcyjne RF, telemetrii i budżetu stosu mają osobne dowody w checkpoincie sprzętowym z 2026-11-27.

## Otwarte bramki — odroczone, niezaliczone

1. H1: native NVS/recovery — pominięty w tym trybie, oczekuje na lokalną kwalifikację operatora. Nie uznawać za zaliczony.
2. H1: brak/stale/invalid wejść, fizyczne LampSafety trip/recovery, dłuższa stabilność oraz potwierdzenie odbiorników. SD: **DEFERRED — brak karty (informacja operatora)**; poprzedni timeout pozostaje obserwacją bez włożonej karty, flash fallback działa. Test SD po włożeniu karty.
3. H2: supervisor-owned quiesce/emergency-lock dopiero po H1.
4. H3: uwierzytelniony kanał preview/activate/status/rollback dopiero po H2.

**Poza tym sprintem:** badawczy MPC ma historycznie **19/22** niezaliczonych trudnych scenariuszy; nie otrzymuje uprawnień do wyjść. Tematy Clay, ML/TimesFM, optymalizacji Maca i rozbudowy driverów nie są aktualnym punktem wznowienia. Historia: [HISTORY.md](HISTORY.md), [CHANGELOG.md](CHANGELOG.md), [research status](simulator/RESEARCH_STATUS.md).

**Dowody:** [finalny checkpoint](checkpoints/2026-11-07-bloomml-documentation-freeze-handoff.md) · [build + NVS matrix](checkpoints/2026-11-07-bloomml-ipo-v2-hardware-qualification-gate.md) · [transakcja IPO](checkpoints/2026-11-07-bloomml-ipo-v2-activation-transaction.md).
