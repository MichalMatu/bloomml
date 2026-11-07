# BloomML — roadmapa rozwoju

**2026-11-07 · aktywny priorytet: IPO v2 — bezpieczne domknięcie konfiguracji na CrowPanel.**

Cel produktu: autonomiczny ESP32-S3 (native ESP-IDF), rzeczywiste czujniki, deterministyczny Rule, RF433 przez OutputSupervisor, wyświetlacz e-ink, telemetria. ML/MPC nadal Shadow/research. Żadnych nowych silników i paneli administracyjnych zamiast istniejącego IPO.

## Stan milestone’ów

| Etap | Status | Co oznacza |
| --- | --- | --- |
| IPO v2 DeviceProfile / edytor / board readiness B5 | **DONE — draft** | Schema, mapowanie, parser, walidacja; bez runtime apply |
| Wspólny RAM Rule/lampa, selector, NVS journal, transaction owner | **DONE — source / host-qualified** | Mint aktywny; produkcyjny `ClosedIpoActivationGate` odrzuca apply |
| Mac C++/Vitest/fault matrix i real-input CrowPanel ESP-IDF build | **PASS — software** | CTest IPO 3/3; Vitest B5 9/9; safety 3/3; ESP-IDF build PASS |
| **H0: podłączony autoryzowany CrowPanel i izolacja wyjść** | **NEXT / BLOCKED** | W ostatnim sprawdzeniu port `/dev/cu.usbserial-120` nieobecny |
| H1: hardware acceptance z fake-locked outputs | **NOT STARTED** | Prawdziwy NVS/flash, reboot, TP357/SCD41, Rule/LampSafety, RAM/stack/watchdog |
| H2: supervisor-owned safe quiesce + emergency lock | **NOT STARTED** | Oddzielny gate z fizyczną kwalifikacją i uprawnieniem operatora |
| H3: authenticated IPO preview/activate/status/rollback API + UI | **NOT STARTED** | Dopiero po H0/H1/H2, bez automatycznego apply z NVS |
| H4: long-run telemetry, real-data calibration, shadow forecasting | **LATER** | Po bezpiecznej działającej ścieżce operatora |

**Wykonuj w tej kolejności.** Rozbudowa API nie może omijać braku fizycznej kwalifikacji. Wybieralne dodatkowe drivery/wyjścia oraz badawcze ML/MPC są osobnymi przyszłymi bramkami, nie przeszkodą do obecnego H0.

## Najbliższy sprint H0/H1 — dokładna kolejność

1. **Najpierw jedynie sprawdź fizyczną dostępność autoryzowanego `/dev/cu.usbserial-120` i potwierdź odłączenie / izolację trzech wyjść.** Nie otwieraj `/dev/cu.usbserial-10` ani nie używaj `/dev/cu.usbserial-1120` bez zgody. Jeśli brak płytki/izolacji, zatrzymaj etap z uczciwym `BLOCKED`.
2. Po potwierdzeniu i wymaganej autoryzacji: zbuduj/zweryfikuj dokładne SHA dla CrowPanel real-input z `GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0`; utrzymaj fizyczne wyjścia fake-locked.
3. Testuj tylko brakujące urządzeniowe scenariusze: realny NVS, slot marker/CRC/przerwanie commit, cold boot/rollback, wybrane podłączone wejścia TP357/SCD41, stale/invalid, termiczne trip/recovery, zasoby i watchdog. Bez nieautoryzowanych komend RF i bez destrukcyjnej próby zasilania działającego growboxa.
4. Rozdziel `source/build pass` od `hardware-qualified`; zapisz dowody exact-SHA. Po hardware PASS przejdź do H2, a dopiero po H2 do H3.

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
