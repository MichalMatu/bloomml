# BloomML — Kaggle 2×Tesla T4 / Mac Local Agent

**Bieżąca roadmapa MPC:** [MPC_ROADMAP.md](MPC_ROADMAP.md). **Status i ryzyka:** [checkpoint](../checkpoints/2026-11-04-bloomml-mpc-resume-complete.md). Kaggle jest etapem **D po lokalnej bramie**, nie zamiennikiem naprawy 19/22 porażek.

## Istniejące połączenie (R4 — historyczny, nie MPC)

`GitHub main SHA (research code) → Mac Local Agent → prywatny notebook Kaggle → manifest → Local Agent → audyt`. Token Kaggle jest wyłącznie na Macu; nigdy w czacie, repo, notebooku czy wyniku testu. Operacje uruchamiane **na żądanie**, bez CI/GitHub Actions.

Sprawdzony notebook **physics R4**, private: https://www.kaggle.com/code/meeehooow/bloomml-r4-physics-stress . Źródło: `notebooks/BloomML_Kaggle_R4_Stress.ipynb`, runner `tools/ml/kaggle_r4_stress_runner.py`. Wymaga dwóch **oddzielnych** `cuda:0` i `cuda:1` (Tesla T4), bez automatycznej podmiany CPU. Kaggle ma ograniczenia quota/runtime i może nie udostępnić obu GPU.

**Dowody R4, nie uruchamiaj ponownie bez powodu:** R2 pilot `84ee2245550b095422a9e5dd6c74834bd1ecc8f4`; R4 V1 `c9e5d2e37e02aa345d25606120416b357175d296`; R4 V2 `05953e0139cfc31c7ba1fde6ba181d6a90bb9fb3`, 8192 komór i 24 424 020 substeps, dwa GPU COMPLETE, min RH 6.65%, max |water residual| ~3.8e-14 kg, 25 CO₂>2000 ppm. [Szczegółowy raport](../checkpoints/2026-11-04-r4-dual-t4-stress.md). To **nie** test MPC, nie empiryczna kalibracja ani benchmark speedup na tym samym CPU.

Przykład diagnostyki istniejącego uruchomienia na Macu (bez sekretów):

```bash
kaggle kernels status meeehooow/bloomml-r4-physics-stress
mkdir -p /tmp/bloomml-kaggle-check
kaggle kernels output meeehooow/bloomml-r4-physics-stress --file-pattern 'r4_stress_manifest' -p /tmp/bloomml-kaggle-check
PYTHONPATH=. python -m tools.ml.research_evidence /tmp/bloomml-kaggle-check/bloomml-r4/r4_stress_manifest.json --root .
```

Nie traktować starego manifestu jako dowodu nowego SHA: weryfikować commit, nazwę notebooka, kompletny run i próbki scalar↔CUDA. Wymagać dwóch kart, bez fabrykowania `COMPLETE`.

## Następny flow — duży eksperyment **MPC**, jeszcze NIE wykonany

1. **Preflight:** świeże `main` SHA i `agent-control:.agent/status/daemon.json`. Local Agent na Macu/Kaggle wg `AGENTS.md` (agent binding, `work_branch=main`, `resources=[]`); klon do katalogu tymczasowego z kontrolą SHA i bez zmian produkcyjnego firmware. Brak Codex, Work, GitHub Actions.
2. **Gate lokalny:** exact-SHA retest hard-stop `9258ae41...`; analiza niepowodzeń, porównanie CPU MPC/threshold/hysteresis, skrócone i wielogodzinne epizody z fault/stale, jawne `FAIL` i `UNKNOWN`. Żadnego dużego Kaggle, gdy bezpieczeństwo/symulator jest niestabilne.
3. **Ocena GPU:** zmierz to samo zadanie na CPU i GPU **na tym samym hostcie**, z FP64 parity, deterministycznymi seedami i raportem time/memory. Obecny beam search to Python + referencyjny model skalarowy: **nie zakładać przyspieszenia CUDA**. GPU można użyć do batch physics, niezależnej oceny scenariuszy lub surrogate training.
4. **Nowe artefakty:** przygotuj *oddzielny* wersjonowany runner i prywatny notebook dla MPC. Obecny notebook R4 nie uruchamia macierzy MPC. Zrób mały pilot na 2×T4: identyczny kod/seed/setup, sumy scenariuszy i weryfikacja, że obie karty wykonały faktyczne obliczenia; test CPU↔GPU oraz powtarzalność.
5. **Dopiero duży eksperyment:** tysiące konfiguracji geometrii/aktorów/klimatu/zakłóceń × wiele seedów, jednakowe warunki dla trzech kontrolerów, różne klasy feasible/infeasible/fault, ograniczenia czasu i budżetu. Protokół porównania ustalony **przed** uruchomieniem; żadnego strojenia na holdout.
6. **Manifest i archiwizacja:** exact SHA kodu+notebook, version/config/seed, num_devices, GPU name, CPU/GPU runtime/parity, VRAM, count/steps/substeps per device, T/RH/VPD/CO₂/PPFD/DLI MAE/RMSE/out-of-tolerance, energia proxy, switches, safety/fallback/timeout, water residual, niedotrzymane ograniczenia, nieudane scenariusze i hash trajektorii. Zweryfikuj manifest lokalnym niezależnym czytnikiem i terminalnym wynikiem Local Agenta. **Nie** zaliczaj `UNKNOWN` jako `PASS`.
7. **Decyzja:** czy MPC wygrywa z baseline w jakich *konkretnych* klasach i z jakim kosztem? Raportuj przegrane i brak akceleracji. Wynik `NUMERICAL_EVIDENCE_PASS` nadal może współistnieć z `CONTROL_QUALITY_FAIL` i `EMPIRICAL_FIDELITY_UNKNOWN`.

**Istotne warunki:** nie przesyłać prywatnych logów/sekretów. Kaggle jest tylko offline compute, nie systemem sterowania. Bez własnych, niezależnych danych nie ma kwalifikacji do fizycznej pracy ESP32-S3, niezależnie od rozmiaru GPU sweep.
