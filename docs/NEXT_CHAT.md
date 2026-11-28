# BloomML — handoff do następnego czatu (2026-11-28)

## Zweryfikowany punkt wznowienia

- Repozytorium `MichalMatu/bloomml`; jedyna gałąź kodu `main`.
- Baseline przed synchronizacją handoffu: `199fa0d43909ef41403f8a5b4164130b5662a59a`. Zakwalifikowany kod M05: `2a3555a77308f55831fe85b74b2ae2fc5898d32a`. Różnica dotyczy wyłącznie dokumentów.
- M01–M05 **CLOSED_SCOPED**, M06 **NOT STARTED**. [M01](checkpoints/2026-11-28-bloomml-m01-mvp-skeleton-audit.md), [M02](checkpoints/2026-11-28-bloomml-m02-ipo-separation.md), [M03](checkpoints/2026-11-28-bloomml-m03-configurable-inputs.md), [M04](checkpoints/2026-11-28-bloomml-m04-configurable-outputs.md), [M05](checkpoints/2026-11-28-bloomml-m05-multiparameter-rule.md).
- Terminalny wynik `agent-control:.agent/results/la-bloomml-m05-final-qualification-20261128-v1.json`: DONE, exit 0, publikacja potwierdzona. Host 62/62, web 117/117 (typecheck/lint/build), Python 13/13, 2560 przypadków decyzji Rule, trzy bramki architektury, build CrowPanel PASS; 79% wolnej partycji. Wyniki dotyczą zakwalifikowanego kodu, nie sprzętu.
- Daemon IDLE, brak zadań bez wyniku; czysty izolowany checkout potwierdzony w terminalnym raporcie M05. Bieżący fizyczny checkout zarządzanego agenta nie był ponownie badany — sprawdź przed lokalnym zapisem.
- Biblioteka procedur `MichalMatu/local-agent`, `main/skills`, SHA użyty przy handoffie `9fd7c7ac439a93926cc0bbb202741bcd8e70e06f`. Polecenia krótkie zgodnie z `skills/CHAT_BOOTSTRAP.md`; zawsze odczytaj aktualny HEAD biblioteki.
- Audyt zakresu M05: istniejący Process, przenośny Rule, profil wire v2 z odczytem v1, obserwator telemetrii; brak zmian w poddrzewie Output i transakcji aktywacji. Ocena wentylacji jest heurystyką; powtórzone kryteria diagnostyki i decyzji (`ClimatePolicy.cpp`, okolice 105–184 i 325–405) to punkt kontroli spójności w M06, nie dowód fizycznej skuteczności. Brak nowych pomiarów kosztu stosu/czasu cyklu; sprawdzić w osobnej kwalifikacji. Konflikt intentów nie dowodzi fizycznej blokady przy dwell/nieznanym stanie.

## Preflight nowej sesji

1. Odczytaj aktualny HEAD `main` i `agent-control:.agent/status/daemon.json`, kolejkę zadań i wyniki. Nie zakładaj SHA handoffu jako aktualnego.
2. Przeczytaj `AGENTS.md`, `docs/README.md`, `docs/CURRENT_STATUS.md`, `docs/CONFIGURATION_FLOW.md`, `docs/PRODUCT_DIRECTION.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md`, niniejszy dokument i checkpointy M01–M05.
3. Tryb GitHub Connector + Local Agent/Chat Bridge. Mac binding `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`; `work_branch=main`, `resources=[]`. Zweryfikuj czystość i właściwy host przed zmianami. Nie powtarzaj zaliczonych prac bez regresji.
4. Nie rozpoczynaj M06 w ramach samego preflight. Po decyzji operatora przejdź do planu poniżej.

## Następny etap M06 — integracja MVP (software)

Cel: użytkownik opisuje wyposażenie i cele klimatu, system przetwarza je przez jeden istniejący przepływ IPO. Weryfikacja software nie nadaje uprawnień fizycznych.

- Prześledź kontrakt: DeviceProfile v2 / JSON / graf / selekcja pomiarów / plan wyposażenia / cele dzień-noc / Process / OutputSupervisor / obserwator. Usuń sprzeczne źródła konfiguracji i potwierdź, że walidator browser oraz firmware rozróżniają poprawny draft, kompatybilność board i gotowość do wykonania.
- Zbuduj brakujące połączenia wyłącznie w istniejącej architekturze, z host/fake ścieżką end-to-end od profilu do decyzji i symulowanego wykonania. Nie buduj równoległego kontrolera ani drugiej transakcji. Mint pozostaje jedynym profilem startowym produkcji.
- Sprawdź warianty wyposażenia, priorytety/fallback czujników, CO₂ dzień/noc, brak urządzeń, zero wyjść, konflikty, nieprawidłowe profile, błędy transportu, spójność statusu i zgodność persistence v1/v2. Nie utożsamiaj żądania, wykonania transportowego i fizycznej obserwacji.
- Ujednolić minimalną ścieżkę konfiguratora, bez sugerowania dostępności fizycznego apply. Większy redesign UX, nowe transporty i ML/MPC poza M06.
- Etapy małe, odwracalne; pełna regresja host, Python parity, web, trzy guardy architektury i build CrowPanel dla zmian kodu. Zapisz exact SHA, dowody, checkpoint i status CLOSED_SCOPED/BLOCKED. Nie przechodź automatycznie do kwalifikacji sprzętowej.

## Niezależne, odroczone bramki sprzętowe

H0 historycznie PASS, H1 PARTIAL, H2/H3 NOT ADMITTED. [Checkpoint fizyczny](checkpoints/2026-11-27-bloomml-hardware-progress.md). Ostatni test fizyczny `2199a75` fake-locked. Native NVS/recovery SKIPPED — TOOL BLOCK, operator-owned local follow-up; nie ponawiaj zablokowanej próby. Brak/stale/invalid, termika, długotrwała stabilność, ACK odbiorników i SD (po włożeniu karty) niezaliczone. H2 supervisor-owned quiesce/emergency-lock, H3 uwierzytelniony preview/activate/status/rollback dopiero po osobnej kwalifikacji. H4 real-data calibration i shadow później.

Bez flashowania, operacji sprzętowych ani otwierania `ClosedIpoActivationGate` w M06. Dla osobno autoryzowanego sprzętowego etapu: wyłącznie port `/dev/cu.usbserial-130`, po świeżym potwierdzeniu izolacji; nie używać portu 10, 1120 wymaga osobnej zgody. RF TX nie jest fizycznym ACK. LampSafety niezależny: trip ≥28°C, recovery ≤26°C przez 10 minut. Rule autorytatywny, OutputSupervisor jedyny normalny właściciel wyjść, brak auto-apply NVS, ML/MPC Shadow.

**Bez Codex, Work, GitHub Actions, nowych gałęzi i force-push.** `agent-control` tylko zadania/wyniki; `gh-pages` tylko publikacja. Nie usuwaj historycznych dowodów.
