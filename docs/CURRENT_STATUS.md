# BloomML — bieżący stan kontrolera

Data: **2026-11-07** · Repo: `MichalMatu/bloomml` · branch: `main`.

**Status:** IPO v2 owner transakcji **HOST QUALIFIED, HARDWARE LOCKED**. Wąskie C++/Vitest przeszły na Macu, ale ESP-IDF real-input i fizyczny sprzęt nie zostały zakwalifikowane ani wgrane. Poprzednie wyniki sprzętowe dotyczą wcześniejszego kodu. **Nie twierdzić, że nowy firmware jest już wdrożony.**

## IPO v2 — owner transakcji i wąska kwalifikacja (2026-11-07)

**SOURCE IMPLEMENTED / HOST QUALIFIED / HARDWARE ACTIVATION LOCKED.**
W kodzie działającym pod real-input utworzono jednego właściciela `IpoProfileActivationTransaction` (nie podłączono żadnego endpointu zapisu). Jego typowana sekwencja to `preview → validate → NVS commit → RAM apply → readback → rollback`. Przed trwałym zapisem wymaga bramki kwalifikacji, która na produkcyjnym CrowPanel jest świadomie **`ClosedIpoActivationGate`** — nie da się z tego firmware włączyć operatorowego v2 ani ominąć `OutputSupervisor`. Po restarcie aktywny jest nadal Mint, niezależnie od poprawnego wpisu w IPO NVS. Konsola czyta faktyczny `RuntimeScheduleProfileOwner`, zapamiętany fingerprint NVS i dowód aktywacji z RAM zamiast wnioskować `applied` z samego zapisu.

Własny rollback dziennika NVS potrafi wskazać poprzednią poprawną kopię lub wrócić do pustej przestrzeni `gb_ipoprof`; przy braku pewnego readbacku transakcja przechodzi w `Quarantined` i żąda zatrzaśnięcia blokady fizycznych wyjść. Nie zmieniono Rule, LampSafety ani ścieżki sterowania RF433.

**Nowe wykonane testy na Mac Local Agent (dokładny kod `977c48d221cd310be70f39653a33d8eb4abd5ebd`):** `make test-ipo` → 2/2 CTest PASS (profile + activation); `pnpm --dir web exec vitest run src/domain/ipo-board-readiness.test.ts` → 9/9 PASS; `git diff --check` PASS. Wynik: `.agent/results/la-bloomml-ipo-v2-transaction-tests-20261107-v2.json`. Testy aktywacji wykorzystują fake journal i bramkę, **nie** weryfikują sprzętowego NVS, resetu ani transportu.

**Nadal otwarte:** celowany ESP-IDF compile real-input CrowPanel, rzeczywisty adapter blokady OutputSupervisor z kwalifikacją exact-SHA, stale/valid TP357+SCD41 i niezależnego zabezpieczenia termicznego lampy, sprzętowe NVS interruption/reboot, readback aktywacji po restarcie, urządzeniowe API dla istniejącego panelu. Brak zgody na odblokowanie fizycznych wyjść w tym sprincie.

## IPO Quality Gate B5 — kod źródłowy, następnie kwalifikacja wąska (2026-11-07)

Dopisano wymagane źródło **SCD41 CO₂** do dopuszczenia obecnego CrowPanel; ograniczono liczbę surowych pomiarów w selektorze i zapobieżono overflow `double → float`; nieaktywny firmware parser JSON v2 ma jawny limit **32 KiB** dopasowany do maksymalnego grafu. Kandydat ze świadomie wyłączonym wyciągiem nie przejdzie trwałej walidacji profilu. W `/` pokazujemy teraz **osobno** poprawność grafu, zgodność z rzeczywistą referencyjną mapą CrowPanel i rozmiar UTF-8, zawsze bez przycisku `apply`. Odczyt statusu w firmware rozdziela Mint w RAM i zapis w NVS; fingerprint NVS buforowany przy starcie, nie kodowany ponownie na stosie konsoli. Powstały dedykowane testy host C++ i web Vitest, a host test został zarejestrowany pod `make test-ipo` — zostały następnie uruchomione w celowanej bramce 2/2 CTest i 9/9 Vitest. Wciąż brak aktywnego profilu operatora i hardware qualification.

Dokładne zmiany i dalszy zakres: [checkpoint B5](checkpoints/2026-11-07-bloomml-ipo-quality-gate-b5.md).

## Jaka jest prawda produkcyjna

- Sterowanie: deterministyczny **Rule**; ML/MPC tylko Shadow/research. Żadnego automatycznego awansu ML.
- Fizyczne wyjścia: wyłącznie `OutputSupervisor`; stałe RF433 gniazda 1 (wyciąg), 2 (lampa), 3 (nawilżacz). Transmisja nie dowodzi rzeczywistego przełączenia odbiornika.
- Bezpieczeństwo lampy: wyłączenie przy **≥28°C**, wznowienie dopiero po **≤26°C przez 10 minut**. Źródłem Safety nadal bezpośredni TP357, bez przełączenia na SCD41 T/RH.
- E-ink/panel pozostają obserwatorami, bez własności sterowania; transport nie może być deklarowany jako sukces przy błędzie lub braku potwierdzenia.

## Co jest w kodzie IPO v2 — ale nie jest jeszcze aktywnym profilem operatora

- `/` to jeden offline konfigurator **DeviceProfile v2**: urządzenia, drivery, role sygnałów, źródła primary/fallback, procesy, wyjścia, powiązania, cele i import/eksport szkicu. v1: `/lab/profile-v1`, badawczy v5: `/lab/v5`, 3D jest podglądem.
- Wspólny walidowany `RuntimeScheduleProfileOwner` trzyma aktualny **wbudowany Mint w RAM** dla Rule i harmonogramu lampy. Nie jest zasilany zapisanym profilem v2.
- Real-input `RuntimeInsideSource` / `RuntimeNearbySource` są w nowym kodzie podłączone do ograniczonego **referencyjnego** grafu IPO: TP357 T/RH (ID 1), SCD41 CO₂ (ID 2), Xiaomi zewnętrzne T/RH (ID 3); valid/finite/age filtrują odczyty według timeoutu. Dynamiczne dodawanie czujników oraz SCD41 T/RH jako fallback pozostają **niewdrożone**.
- Istnieje ścisły parser JSON v2 → bounded typowany graf, walidacja możliwości CrowPanel, kod wersjonowanego binary codec/CRC oraz dwuslotowy magazyn `gb_ipoprof` z zachowaniem poprzedniej dobrej kopii. `IpoProfileStagingCoordinator` ma metodę prepare/stored-only/readback, ale **nie jest wywoływany przez aktywny runtime**.
- Podczas startu kod **tylko odczytuje** stan IPO NVS journal. Konsola `status` rozdziela `ipo_store` (stan zapisu), `ipo_profile` (aktywne cele wbudowanego Mint, generation/schedule_hash) i `ipo_input` (źródło/wiek/ważność pomiaru). Profil v2 **nie jest zastosowany**: `profile_v2_applied=0`.

## Otwarte bramki

1. Wąska kwalifikacja nowego kodu: kompilowalność, zachowanie Rule przy granicy stale/invalid, niezależność lamp Safety, korupcja/przerwanie zapisu NVS, recovery i reboot.
2. Jeden uprzywilejowany, synchronizowany owner transakcji konfiguracji: validate → stage → journal commit → bezpieczne apply → active hash/readback → rollback. Nie odblokowywać przed kwalifikacją.
3. Wybieralne rzeczywiste fallbacki i kolejne drivery/output role przez jawnie wspierane adaptery oraz istniejący `OutputSupervisor`; bez arbitralnych komend z JSON.
4. Rzeczywiste API do istniejącego panelu, odczyt tego samego stanu w e-ink, kalibracja symulatora prawdziwymi trajektoriami.

Pełna mapa IPO, wspierane i projektowane role: [CONFIGURATION_FLOW.md](CONFIGURATION_FLOW.md). Kontynuacja bez powtarzania wykonanych prac: [NEXT_CHAT.md](NEXT_CHAT.md). Dowód kodowy: [checkpoint 2026-11-07](checkpoints/2026-11-07-bloomml-ipo-runtime-bridge.md).


## MPC / Kaggle research now in main (2026-11-07)

The R4 numerical simulator, Kaggle runners/notebooks, external greenhouse dataset tooling, and experimental MPC have been merged into `main` while staying exclusively under `tools/ml/`, `tests/`, `notebooks/` and `docs/simulator/`. Code inclusion is **not** an approval for physical actuation. Latest research record: [research status](simulator/RESEARCH_STATUS.md), [MPC roadmap](simulator/MPC_ROADMAP.md) and [2026-11-04 exact-SHA checkpoint](checkpoints/2026-11-04-bloomml-mpc-resume-complete.md).

Historical MPC challenge matrix: **3/22 PASS, 19/22 FAIL**; a 60-minute representative MPC episode was PASS, but physical model calibration/own growbox logs and active MPC hardware approval remain absent. Separate research branch history is preserved as a merge parent. New work goes directly on `main`, keeping the research / runtime ownership boundary.

## Display / hardware baseline

The production CrowPanel display path is integrated and stable at the software boundary:

- native ESP-IDF SSD1680 backend remains the sole hardware/framebuffer owner;
- exactly one 296x128 / 4736-byte framebuffer is retained;
- Clay rendering is isolated behind growbox-owned C++17-compatible semantic types;
- display work remains asynchronous;
- reduced partial refresh is integrated;
- Home/Back/Previous/Next/OK input and the read-only four-page chooser are integrated without taking control/output ownership.

Existing exact-SHA hardware evidence showed healthy display refresh, stable heap/PSRAM and no panic/brownout/watchdog failure with physical outputs fake-locked. Completed phase-by-phase evidence belongs in `docs/HISTORY.md`, `docs/CHANGELOG.md` and Git history rather than this status file.

Remaining physical qualification is intentionally explicit rather than disguised as software debt:

- SCD41 behavior must be re-qualified with the sensor physically present;
- manual key navigation and visual/ghosting checks require operator interaction;
- destructive or real-output hardware qualification must follow the current `AGENTS.md` hardware rules.

## Performance state

The completed local build/test acceleration pass is merged. Current defaults remain:

- ESP-IDF ccache enabled by default;
- `HOST_BUILD_JOBS=4`.

No CPU-vs-Apple-GPU ML A/B was run before the pause. The current ML dependency set does not pin `tensorflow-metal`, so GPU availability must be probed on the new host rather than assumed.

The canonical benchmark history and new-Mac comparison procedure are in `docs/PERFORMANCE_HANDOFF.md`. Do not change dependencies, worker counts or device placement before recording the fixed baseline.

## Resume order

When the M1 Pro / 32 GB machine is available:

1. fetch fresh `main` and fresh `agent-control` daemon state;
2. read `AGENTS.md`, `docs/README.md`, this file, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md` and `docs/PERFORMANCE_HANDOFF.md`;
3. rerun the fixed historical build/test baseline on the new Mac for an apples-to-apples host comparison;
4. establish a fresh current-`main` CPU/RAM/cache baseline;
5. probe the installed TensorFlow device/plugin state before deciding whether a CPU/GPU A/B is possible;
6. only after those baselines consider higher build parallelism, Metal dependencies, or another product slice;
7. re-open physical SCD41/button qualification only when the required hardware/operator interaction is available.

Historical Stage/Phase handoffs, closed audit narratives and benchmark task ids are not active state. Recover them from Git history when evidence is needed instead of expanding this file again.

