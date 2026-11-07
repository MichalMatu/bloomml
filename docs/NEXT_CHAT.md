# BloomML — handoff po IPO Runtime Bridge 4 (2026-11-07)

## Wejście

- Repo `MichalMatu/bloomml`, źródło tylko `main`, `agent-control` dla Local Agent, `gh-pages` publikacyjne.
- Startuj od **świeżego HEAD**, nie utrwalaj SHA ze starej rozmowy. Sprawdź stan `.agent/status/daemon.json` przed nowym zadaniem Local Agent. Nie używaj Codex, Work ani GitHub Actions.
- Czytaj `AGENTS.md`, `docs/README.md`, `docs/CURRENT_STATUS.md`, `docs/CONFIGURATION_FLOW.md` i `docs/ARCHITECTURE.md` przed zmianami.
- Operator wprost prosił **bez ponawiania testów i ciężkich kompilacji**; w Bridge 1–4 żadnych nie wykonano. Nowy kod pozostaje **UNVERIFIED**. Przed fizycznym wgraniem nowego firmware konieczna nowa celowana kwalifikacja dotkniętej ścieżki — nie historycznych badań ML.

## Zamknięte źródłowo — nie powtarzać

1. Jeden offline edytor `DeviceProfile v2` na `/` wraz z driver catalog, signal registry, process/effect/output links, migracja z v1. Nowszy v2 szkic **nie jest aktywną konfiguracją**; v1 zachowany `/lab/profile-v1`, v5 research `/lab/v5`.
2. Bounded `lib/ipo_profile`: C++ `IpoGraph`, `IpoBoardAdmission` wymagający pełnych pięciu referencyjnych połączeń Mint, `IpoSignalSelector` ze świeżością i priorytetami. Admission = referencja, **apply_authorized=false**.
3. Wspólny `RuntimeScheduleProfileOwner` dla Rule i harmonogramu lampy, wbudowany Mint pozostaje aktywny.
4. Bounded typowany firmware JSON parser `IpoProfileJsonDecode`, full-profile `IpoProfileWire` z magic/version/CRC, osobna `IpoProfileNvsJournal` dwuslotowa, dormant `IpoProfileStagingCoordinator` stored-only. Nie wywoływać NVS `commit` jako substytutu aktywnego apply.
5. W **real-input** nowy selektor jest podpięty do TP357 T/RH, SCD41 CO₂, Xiaomi T/RH przy zachowaniu osobnej lamp Safety TP357. Zapisany profil NVS jest na boot wyłącznie odczytywany i **nigdy automatycznie stosowany**.
6. `Stage28ServiceConsole status` rozróżnia `ipo_store` (zapis), `ipo_profile` (aktywny Mint/schedule_hash) i `ipo_input` (valid, source id, age, fallback poprzedniego cyklu). Żadne z nich nie potwierdza fizycznego stanu RF433.
7. Dokumentacja prawdy: `docs/CONFIGURATION_FLOW.md`; checkpoint `docs/checkpoints/2026-11-07-bloomml-ipo-runtime-bridge.md`.

## Zamknięte po Bridge 4 — B5 IPO Quality Gate

- `IpoBoardAdmission` wymaga kwalifikowanego wejścia CO₂ obok TP357 T/RH i RTC; brak źródła mimo safety-link odrzuca profil. `IpoSignalSelector` ogranicza liczbę wejść i blokuje overflow przy konwersji do `float`. `IpoProfileWire` blokuje deklarację niedostępnego wyciągu.
- `IpoProfileJsonDecode` limit UTF-8 32 KiB; `web/src/domain/ipo-board-readiness.ts` plus istniejący edytor `/` pokazują preflight obecnego CrowPanel, 80 bajtów na etykietę, miejsca i brak możliwości apply. Eksport kompaktowego JSON z tym samym limitem.
- Jedna observer-only projekcja `IpoProfileReadback` pokazuje active Mint RAM vs stored NVS. NVS odczyt buforuje CRC32 zapisanego rekordu, a konsola ma czytelne `ipo_health` + `ipo_readback` bez dodatkowego dużego bufora na stosie i bez zmiany dotychczasowych wierszy.
- Dodano `test/test_ipo_profile/test_main.cpp` (C++) oraz `web/src/domain/ipo-board-readiness.test.ts` (Vitest) — **testów nie wykonywano**; nie powtarzać ich implementacji.
- Dokładny zakres w `docs/checkpoints/2026-11-07-bloomml-ipo-quality-gate-b5.md`.

## Jeden następny sprint

- Przed aktywacją fizyczną wykonać **skoncentrowaną** kwalifikację nowych ścieżek: valid/stale TP357 i SCD41, Rule+lamp Safety, kodowanie i power-loss NVS, rozróżnienie stored/active, ponowny start. Nie powtarzać starych historycznych testów; gdy operator nadal wyklucza testy, pozostać przy kodzie dormant/read-only.
- Następnie przy jednym właścicielu i po spełnieniu bramek: `preview → validate → stage NVS → safe apply → readback active hash → rollback`, bez drugiego silnika wyjść. Tylko jawnie wspierane endpointy i adaptery; `OutputSupervisor` i bezpieczeństwo nadrzędne.
- Dopiero później sieciowe/lokalne API do już istniejącego operator UI i e-ink observer; brak fałszywego `applied` na podstawie zapisanych bajtów.
- ML pozostaje Shadow, symulator/Kaggle offline, MPC research nie ma prawa aktywacji.

**Konkretny stan:** nowy firmware zmienia selekcję/odrzucanie starych pomiarów, ale nie został skompilowany ani wgrany. Nie deklarować PASS/uruchomienia.
