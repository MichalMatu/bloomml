# BloomML — handoff po IPO v2 Activation Transaction (2026-11-07)

## Wymagany start
- Repo `MichalMatu/bloomml`; źródło tylko `main`, `agent-control` tylko Local Agent, `gh-pages` publikacyjne.
- Odśwież HEAD `main` i stan `agent-control:.agent/status/daemon.json` przed jakimkolwiek zapisem/zadaniem. Nie zakładaj SHA handoff jako aktualnego.
- Czytaj `AGENTS.md`, `docs/README.md`, `docs/CURRENT_STATUS.md`, `docs/CONFIGURATION_FLOW.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md` i `docs/checkpoints/2026-11-07-bloomml-ipo-v2-activation-transaction.md`.
- Bez Codex, Work, GitHub Actions, nowej gałęzi, force-push, powtarzania zamkniętych sprintów i ciężkich niecelowanych buildów.

## Zamknięte — nie powtarzać
- B5 admission/preflight, bounded JSON/wire/NVS journal, Readback i real-input selector; wąska bramka B5 została wykonana.
- Jeden nowy `IpoProfileActivationTransaction`, `ClosedIpoActivationGate`, NVS `rollbackTo`, aktywny vs zapisany status w konsoli. Wpisane w real-input, ale żadna droga do aktywacji nie jest odblokowana.
- Wąskie kwalifikacje Mac Local Agent na kodzie `977c48d221cd310be70f39653a33d8eb4abd5ebd`: `make test-ipo` -> 2/2 CTest PASS; `pnpm --dir web exec vitest run src/domain/ipo-board-readiness.test.ts` -> 9/9 PASS; `git diff --check` PASS. Evidence: `.agent/results/la-bloomml-ipo-v2-transaction-tests-20261107-v2.json`. C++ transakcja używa *fake* journal; to nie jest fizyczny NVS.
- Nie przeprowadzono ESP-IDF CrowPanel build, hardware flash/reset/testu fizycznego. Nie deklarować production PASS.

## Następny jeden sprint — kwalifikacja urządzeniowa i port bezpieczeństwa
1. Skupiona weryfikacja produkcyjnej ścieżki real-input CrowPanel (minimalny ESP-IDF build na exact SHA z odłączonym/fake-locked RF) i lokalnych źródeł TP357/SCD41, invalid/stale, Rule, niezależnego TP357 LampSafety. Utrzymać progi >=28°C trip, <=26°C przez 10 min recovery.
2. Wąskie fault-injection dla **rzeczywistego** adaptera NVS: dwufazowy zapis, zerwanie energii przed/po flipie, corrupt slot, fallback, rollback pustego i poprzedniego slotu, reboot `stored != active`, zasoby/stack.
3. Dopiero po kwalifikacji zaprojektować i podłączyć implementację `IpoActivationSafetyGate` przez istniejący OutputSupervisor quiesce/emergency-lock. Zablokować wejście bez uprawnień i potwierdzenia operatora. Żadnego bezpośredniego RF/GPIO w transakcji.
4. Wprowadzić minimalny, uwierzytelniony lokalny kanał preview/activate/status do **istniejącego** konfiguratora, dopiero po kwalifikacji. Zachować prawdę `stored` vs `active` vs fizyczny RF state.
5. Osobny checkpoint exact SHA, wyniki tylko nowych testów, resztkowe blokery i handoff.

## Bezwzględne granice
Rule jest autorytetem. ML/MPC Shadow. `OutputSupervisor` jedynym właścicielem wyjść. LampSafety niezależna. `ClosedIpoActivationGate` ma pozostać zamknięta dopóki nie przejdzie osobna kwalifikacja sprzętowa i operator nie autoryzuje aktywacji. Brak automatycznego apply z NVS przy restarcie.
