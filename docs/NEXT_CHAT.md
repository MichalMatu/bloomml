# BloomML — handoff do następnego czatu (2026-11-07)

## JEDEN NASTĘPNY KROK — ZACZNIJ OD TEGO

**Podłączyć autoryzowany CrowPanel do Maca i potwierdzić fizyczną izolację wszystkich wyjść.**

Ostatni Mac Local Agent zgłosił **`/dev/cu.usbserial-120` ABSENT**. Dopóki port i faktyczna izolacja trzech RF433 wyjść nie będą potwierdzone, **nie flashuj, nie resetuj, nie wykonuj testu power-cut, nie zmieniaj firmware i nie odblokowuj IPO v2**. Gdy płytka nadal jest nieobecna, zgłoś `BLOCKED / HARDWARE_ABSENT`, nie zastępuj tego ponawianiem już zaliczonych testów ani budowaniem przedwczesnego API.

## Software audit / repair — ZAMKNIĘTY

Nie powtarzać bez regresji: commity `cf1c434` / `196c305` / `4aadf5f` zamknęły cztery TypeScript błędy, 96 naruszeń lint oraz obsługę niebezpiecznego/nieświeżego odświeżania diagnostyki. Dowody: web **114/114 Vitest** + typecheck/lint/build PASS, panel **135/135 pytest** PASS, host C++/display PASS, bazowy IPO **3/3** PASS. [Checkpoint naprawczy](checkpoints/2026-11-07-bloomml-general-repository-repair.md). Nie wykazano fizycznej izolacji ani nowego ESP-IDF builda na aktualnym HEAD; kolejnym krokiem pozostaje H0.

## Preflight nowej sesji

1. Repo **`MichalMatu/bloomml`**: odczytaj **aktualny** HEAD `main` i `agent-control:.agent/status/daemon.json`; nie zakładaj SHA handoff jako bieżącego. Mac Local Agent binding: `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`, `work_branch=main`, `resources=[]`.
2. Przeczytaj `AGENTS.md`, `docs/README.md`, `docs/CURRENT_STATUS.md`, `docs/CONFIGURATION_FLOW.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md` i [checkpoint fizyczny](checkpoints/2026-11-07-bloomml-documentation-freeze-handoff.md) i [nowy checkpoint naprawczy](checkpoints/2026-11-07-bloomml-general-repository-repair.md).
3. Sprawdź **tylko autoryzowany port** `/dev/cu.usbserial-120`. **Nigdy** `/dev/cu.usbserial-10`; `/dev/cu.usbserial-1120` wymaga odrębnej zgody. Najpierw potwierdź fizyczną izolację wyjść, nie wyciągaj wniosku o izolacji z samego flag/config builda.

## Co jest zakończone — NIE POWTARZAJ

- IPO v2 B5: walidacja board/readiness; Vitest **9/9 PASS**.
- Jedna transakcja `IpoProfileActivationTransaction`; NVS CRC/two-slot/readback/rollback, diagnostyka stored vs active, produkcyjny `ClosedIpoActivationGate`.
- `make test-ipo` **3/3 CTest PASS** na `45f3527`; obejmuje adapter NVS z fault injection przy **mock** ESP-IDF API. LampSafety / BLE stale-Rule / output lifecycle **3/3 PASS** na `347bcca`.
- CrowPanel N8R8 real-input/e-ink **ESP-IDF BUILD PASS** na `1a1b3de8` (bin `0xd1e00`, 80% app partition free). Nie jest to test hardware ani flash.
- Końcowy poprzedni commit `c4bf62cf70b5197f5b19906cf48175a75719785e`: dokumentacja/closed gate check PASS, repo czyste.
- Historyczne Bridge 1–4/B5/Clay/ML nie są zadaniami do ponownego otwarcia z samego powodu starszych opisów.

Dowody: [checkpoint kwalifikacji](checkpoints/2026-11-07-bloomml-ipo-v2-hardware-qualification-gate.md) i [checkpoint transakcji](checkpoints/2026-11-07-bloomml-ipo-v2-activation-transaction.md).

## Dopiero po H0 — fizyczna obecność + izolacja

1. Z autoryzacją operatora i izolowanymi wyjściami wykonaj rzeczywisty `ESP-IDF` hardware gate na exact SHA z `GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0`: NVS flash/reboot/slot recovery, TP357/SCD41 brak/stale/invalid, niezależny LampSafety, Rule, heap/PSRAM/stack/watchdog. Nie wykonuj ryzykownych odłączeń zasilania bez świadomej zgody i izolacji.
2. Po PASS zaprojektuj supervisor-owned `quiesce / emergency-lock`, bez osobnych zapisów RF/GPIO, z zatrzaśniętym quarantine przy niepewnym rollbacku. Zachowaj wyłączną własność OutputSupervisor i niezależne safety.
3. Dopiero potem dodaj **uwierzytelniony** lokalny preview/activate/status/rollback do istniejącego konfiguratora. `tools/panel` jest historycznym serwisem demo i nie stanowi automatycznie produkcyjnego API. Każdy status musi odróżniać `stored`, `active` i fizyczną obserwację.
4. Zapisz nowy checkpoint exact-SHA, test matrix, blokery i handoff.

## Zakazy i stałe kontrakty

`main` tylko dla source/docs, `agent-control` tylko zadania/wyniki Local Agent, `gh-pages` publikacja. **Bez Codex, ChatGPT Work, GitHub Actions, nowych gałęzi i force-push.** Bez automatycznego apply z NVS. Produkcyjny `ClosedIpoActivationGate` pozostaje zamknięty aż do niezależnej kwalifikacji i jawnej autoryzacji. Rule jest autorytetem, ML/MPC Shadow. Lampa: trip ≥28°C; recovery ≤26°C przez 10 minut. RF433 TX nie jest fizycznym ACK.
