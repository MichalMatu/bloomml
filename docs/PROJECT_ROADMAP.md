# BloomML — roadmapa rozwoju

**2026-11-28 · aktywny priorytet software: M06 — integracja MVP bez aktywacji fizycznej.**

Cel produktu: autonomiczny ESP32-S3 (native ESP-IDF), rzeczywiste czujniki, deterministyczny Rule, RF433 przez OutputSupervisor, wyświetlacz e-ink, telemetria. ML/MPC nadal Shadow/research. Żadnych nowych silników i paneli administracyjnych zamiast istniejącego IPO.

## Stan milestone’ów

| Etap | Status | Co oznacza |
| --- | --- | --- |
| IPO v2 DeviceProfile / edytor / board readiness B5 | **DONE — draft** | Schema, mapowanie, parser, walidacja; bez runtime apply |
| Wspólny RAM Rule/lampa, selector, NVS journal, transaction owner | **DONE — source / host-qualified** | Mint aktywny; produkcyjny `ClosedIpoActivationGate` odrzuca apply |
| Mac C++/Vitest/fault matrix i real-input CrowPanel ESP-IDF build | **PASS — software** | CTest IPO 3/3; Vitest B5 9/9; safety 3/3; ESP-IDF build PASS |
| General software audit repairs | **DONE — scoped** | Web 114 Vitest + typecheck/lint/build, panel 135 pytest, host/display PASS; physical gates unchanged. [Checkpoint](checkpoints/2026-11-07-bloomml-general-repository-repair.md) |
| **H0: podłączony autoryzowany CrowPanel i izolacja wyjść** | **PASS** | Port 130 i izolacja potwierdzone |
| H1: hardware acceptance z fake-locked outputs | **PARTIAL** | Normalne wejścia, zasoby, fallback PASS; NVS native SKIPPED (tool block), stale/invalid, termika i pozostałe testy sprzętowe DEFERRED; SD bez karty |
| H2: supervisor-owned safe quiesce + emergency lock | **NOT STARTED** | Oddzielny gate z fizyczną kwalifikacją i uprawnieniem operatora |
| H3: authenticated IPO preview/activate/status/rollback API + UI | **NOT STARTED** | Dopiero po H0/H1/H2, bez automatycznego apply z NVS |
| H4: long-run telemetry, real-data calibration, shadow forecasting | **LATER** | Po bezpiecznej działającej ścieżce operatora |

**Kolejność bramek sprzętowych H0–H4 pozostaje obowiązująca.** M06 to niezależna integracja software, bez dopuszczenia H2/H3 ani fizycznego apply. Rozbudowa API nie może omijać fizycznej kwalifikacji. Nowe rzeczywiste drivery i ML/MPC to osobne przyszłe etapy.

## Szkielet software MVP

M01 audyt, M02 separacja IPO, M03 wymienne wejścia, M04 wymienne wyjścia, M05 wieloparametrowy Rule — **CLOSED_SCOPED**. [Dowody M05](checkpoints/2026-11-28-bloomml-m05-multiparameter-rule.md).

M06 — **NOT STARTED**: zintegrować konfigurację, plan wykonawczy i istniejący runtime na host/fake; ujednolicić znaczenie draft, validation, board readiness i status; zachować Mint oraz zamkniętą aktywację. Szczegóły i granice: [NEXT_CHAT.md](NEXT_CHAT.md).

## Kolejny sprint sprzętowy H1 — odroczony

1. H0 zakończone; aktualny autoryzowany port 130, fizyczna izolacja potwierdzona.
2. Nie powtarzać pozytywnych prób bez regresji. [Aktualne dowody](checkpoints/2026-11-27-bloomml-hardware-progress.md).
3. Native NVS/recovery pominięty w ChatGPT na decyzję operatora; lokalny follow-up pozostaje OPEN. Bieżący sprint software zamknięty w zweryfikowanym zakresie; dalsze niezależne prace tylko na nowe zlecenie.
4. Pozostała kwalifikacja H1 odroczona na decyzję operatora: stale/invalid, termika, dłuższa stabilność, potwierdzenie odbiorników i test SD po włożeniu karty (obecnie brak). H1 pozostaje PARTIAL, H2/H3 niedopuszczone.
5. Historyczna regresja bufora telemetrii `a813a25`, 3/3 host suites PASS; nie powtarzać bez regresji. Najnowsze dowody software w checkpoincie M05.

## Nienaruszalne warunki akceptacji

- Rule pozostaje autorytetem; ML/MPC tylko Shadow.
- OutputSupervisor jest jedynym właścicielem normalnych fizycznych komend; transport RF nie stanowi potwierdzenia fizycznego wykonania.
- TP357 LampSafety niezależny: ≥28°C trip, ≤26°C przez 10 min recovery. Brak czujnika ma kończyć się bezpiecznie.
- `stored NVS` nie równa się `active RAM`; brak auto-apply przy boot. Nie odblokowywać `ClosedIpoActivationGate` bez osobnej kwalifikacji i jawnej zgody operatora.
- Wyświetlacz i UI są obserwatorami; endpoint operatora musi być uwierzytelniony i wpięty w jeden owner transakcji.

## Inne kierunki — zachowane, ale nieaktywne w tym handoffie

- Clay/e-ink: własność framebufferów i async pipeline już zintegrowana; ręczna kwalifikacja pięciu przycisków i SCD41 na fizycznej płytce pozostaje osobna.
- Realne trajektorie, replay i telemetryczna poprawność decyzji przed kolejną kalibracją symulatora.
- MPC: historyczna trudna macierz **3/22 PASS, 19/22 FAIL**; zero uprawnień produkcyjnych. [Research status](simulator/RESEARCH_STATUS.md).
- TimesFM: tylko offline teacher/benchmark; opcjonalny ESP32-sized student dopiero na dowodach z realnych danych, w Shadow, bez sieci w produkcji.
- Rozszerzanie driverów i efektorów wyłącznie na podstawie realnych potrzeb, capability admission i późniejszej osobnej kwalifikacji.

Historia szczegółowych Bridge 1–4, wcześniejsze plany UI, eksperymenty i zamknięte audyty pozostają w [HISTORY.md](HISTORY.md), [CHANGELOG.md](CHANGELOG.md) oraz checkpointach, **nie jako ponownie otwarte zadania**.

**Źródło wykonawcze dla powrotu:** [NEXT_CHAT.md](NEXT_CHAT.md) · [CURRENT_STATUS.md](CURRENT_STATUS.md) · [finalny checkpoint](checkpoints/2026-11-07-bloomml-documentation-freeze-handoff.md). Branch: `main`; `agent-control` tylko Local Agent; `gh-pages` tylko publikacja. Bez nowych gałęzi, Codex, Work, GitHub Actions i force-push.
