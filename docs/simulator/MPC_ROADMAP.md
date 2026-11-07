# BloomML MPC — aktywna roadmapa badawcza

**Jedyny plan kolejnych prac MPC, 2026-11-04.** Przed rozpoczęciem: [pełny checkpoint](../checkpoints/2026-11-04-bloomml-mpc-resume-complete.md). Położenie kodu: `tools/ml/mpc/`. Kod badań jest na `main`, ale izolowany pod `tools/ml/`; nie dotykać firmware, Growclip, Rule ani OutputSupervisor.

| Kolejność | Zadanie | Gate / dowód ukończenia |
| --- | --- | --- |
| **P0.1** | Exact-SHA retest ostatnich awaryjnych hard-stopów, stateful dwell/slew/interlocków po `9258ae41...` | Jednostkowe + R4 integration PASS, kontrola bezpieczeństwa pre-existing ON, rejection reasons i testy negatywne, zapis logu/SHA; dotychczas **UNKNOWN** |
| **P0.2** | Atrybucja 19/22 porażek w macierzy 11×2×3 | Metryki osobno warm-up i steady state, reachable/infeasible, fault/stale/limited-capacity; nie zmieniać gate dla ukrycia FAIL; baseline identyczne seedy |
| **P0.3** | Minimalne ulepszenia MPC: feasibility, priority i disturbances, bez monolitu | Wersjonowany before/after na tej samej macierzy; brak regresji interlock/safety; raport przypadków, gdzie baseline wygrywa |
| **P1.1** | Wielogodzinny/wielodniowy E2E, tylko CPU na start | 2→6→24→72 h ze stopniowanym budżetem; T/RH lub VPD/CO₂/PPFD, DLI, energie proxy, switches, constraints, fallback, timeout, water residual; multi-seed dopiero po małym smoke |
| **P1.2** | Rozbudowa obserwowalności: księga energii/CO₂, opóźnione i brakujące odczyty, deterministyczny replay, sensor age | Jawne brakujące pomiary, brak fikcyjnego ACK, osobny truth ledger i metryki predykcji; provenance z source SHA/seed |
| **P1.3** | CPU/GPU decision: ta sama fizyka i workload na jednym hoście | Mierzone runtime i peak RAM/VRAM, FP64 parity, FP32 osobno, udział GPU w czasie; bez deklaracji speedup bez pomiaru |
| **P2 / D** | **Kaggle 2× Tesla T4 — duża macierz MPC** | Nowy wersjonowany runner/notebook (istniejący R4 jest tylko physics), dwa aktywne GPU, podział konfiguracji/seedów, tysiące trajektorii, baseline i worst-case, wyniki/parity na manifestach; jedynie gdy lokalne gates i koszt uzasadniają |
| **P2 / E** | Utrwalenie pasywnego ML shadow contract, bez zmian firmware | Proposals, sensor validity/freshness, hypothetical Rule verdict, applied/rejected/timeout/missing-ACK, fallback na atrapach, brak wyjść fizycznych |
| **P3** | Empiryczna kalibracja i walidacja własnej komory | Niezależne rzeczywiste logi, actuators applied/energy/flows i held-out days; do tego czasu **fidelity UNKNOWN / hardware BLOCKED** |

## Reguły przejścia na Kaggle

1. Zamknij P0.1–P0.3 i wykonaj przynajmniej reprezentatywne krótkie/średnie P1.1 bez regresji bezpieczeństwa. **Nie wymagaj 100% PASS dla celowo nieosiągalnych konfiguracji**, ale wymuszaj uczciwą klasyfikację feasible / infeasible / fault / unknown i zapis negatywnych wyników.
2. Zmierz CPU na identycznym zadaniu i uzasadnij, co GPU przyspiesza: batched R4, ocenę scenariuszy lub surrogate training; zwykłe Python beam search może wcale nie zyskać.
3. Zbuduj oddzielny runner + private notebook i mały pilot z obydwoma T4; wymagaj CPU↔GPU parity, reproducibility i manifestu. Jeśli brak akceleracji, rozważ CPU bez symulowania sukcesu GPU.
4. Dopiero potem tysiące różnych geometrii/klimatów/mocy/seedów, identyczne baseline, znane tolerancje, percentyle MAE/RMSE, out-of-tolerance fraction, hard violations, energy proxy, switches, timeout, failure categories, CPU/GPU parity i czas.
5. Zgłoś `PASS/FAIL/BLOCKED/UNKNOWN` rozdzielnie: **software/numerical evidence**, **synthetic control quality**, **empirical fidelity**, **hardware approval**. Dane syntetyczne nie są dowodem uprawowym.

**Kaggle jest obowiązkowym kolejnym etapem po lokalnym gate, nie automatycznie teraz.** Konkretny operacyjny flow w [KAGGLE_EXECUTION.md](KAGGLE_EXECUTION.md). Starsze R4 runy nie wymagają ponownego uruchomienia. Każde zamknięte zadanie: test + exact SHA + commit na `main` (wyłącznie kod badań) + krótkie dopisanie wyniku do checkpointu, bez powielania roadmapy.
