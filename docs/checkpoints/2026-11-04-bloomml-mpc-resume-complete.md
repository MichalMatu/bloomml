# BloomML — kompletny checkpoint wznowienia MPC

**Data:** 2026-11-04 · **Tryb:** offline research, tylko symulacja.  
**Repo:** `MichalMatu/bloomml` · **branch:** `research/simulator-fidelity-kaggle-20261103`.  
**HEAD kodu przed porządkowaniem dokumentacji:** `9258ae41ca50d82e704b67243233a6388b17552b`.  
`main`: `51c90cd3290efde230f0c929e93aaf620e0de106` (bez zmian).  
Local Agent na `agent-control`: `idle`, `current_task_id=null`, ostatni sprawdzony zapis `2026-11-04T01:51:26Z`; ten odczyt nie gwarantuje przyszłej bezczynności.

**JAK WRÓCIĆ:** przeczytaj `AGENTS.md`, ten dokument, [aktywną roadmapę MPC](../simulator/MPC_ROADMAP.md), [kontrakt MPC](../simulator/MPC_RESEARCH.md) i [flow Kaggle](../simulator/KAGGLE_EXECUTION.md). Odczytaj ponownie HEAD branchy i Local Agent; nie zakładaj, że SHA powyżej pozostanie aktualne. Historycznych eksperymentów nie powtarzaj bez hipotezy diagnostycznej.

## 1. Zamknięte prace i granica dowodu

- **R4 model fizyczny 0D** (`tools/ml/physics/growbox_dynamics.py`, niezmieniany podczas sprintu MPC): termika powietrza/ścian/liści/podłoża, RH/VPD, CO₂, PPFD/DLI, para/mgła, kondensacja i księga wody. Wewnętrzne inwarianty i stabilność numeryczna ≠ kalibracja ani empiryczna zgodność z konkretnym growboxem.
- **Sprint A, model wirtualnych urządzeń:** `tools/ml/mpc/devices.py` — interlocki, ograniczenia mocy, slew, minimum on/off, awarie, rozróżnienie żądania i zastosowania. Ostatni commit wprowadził priorytety awaryjne i hard-stop przełamujący dwell; **wymaga retestu na exact HEAD `9258ae41...`**.
- **Sprint B, algorytm:** `contracts.py` (T, RH **XOR** VPD, CO₂, PPFD/DLI, tolerancje); `optimizer.py` (deterministyczne seeded beam search, planowanie horyzontu, koszt klimatu/energii/przełączeń, limit ewaluacji, fallback); niezależny solver R4 jako model predykcji, bez podwójnej fizyki w kontrolerze.
- **Sprint C, zamknięta pętla:** `lab.py`, `cli.py`, `scenario_matrix.py` — MPC, threshold, prawdziwa stanowa histereza, porównanie na tych samych warunkach, metryki/ślad/manifest, 11 klas scenariuszy.
- **Sprint E, wyłącznie kontrakt shadow:** `shadow_contract.py` — propozycja → hipotetyczny Rule → hipotetyczny OutputSupervisor, odrzucenia/timeout/brak potwierdzenia; żadnych fizycznych komend, połączeń z firmware ani realnego ACK.
- **Sprint D, MPC na Kaggle:** **NIE URUCHOMIONO**. Starszy eksperyment **R4 physics** rzeczywiście wykonał 8192 komory i 24 424 020 podkroków na 2 osobnych T4; NIE jest benchmarkiem MPC ani dowodem akceleracji jego optymalizatora.

Wersje kodu MPC w Git:
`1feaa6af3d2657faa3a3cbb54ee7cfcc24cc6d2a` → `227c7be00265f3a360b493497a1231fdc72a73a4` → `02ac7e244272db69e6649a37fd9c5ad272185723` → `2ad2177943c4e3a50a4ad094bf939600f1ce14bf` → **`9258ae41ca50d82e704b67243233a6388b17552b`**. Dokumentacyjny commit po tym checkpointcie nie jest dowodem nowego wykonania kodu.

## 2. Zweryfikowane testy i eksperymenty — tylko wskazane SHA

| Źródło / dowód | Testy | Wynik |
| --- | --- | --- |
| `1feaa6af...` · `la-bloomml-mpc-r4-integration-pilot-20261104-v2.json` | 12/12 PASS | 3 min, poprawna pierwsza integracja MPC z R4 |
| `02ac7e...` · `la-bloomml-mpc-r4-hourly-20261104-v2.json` | 23/23 PASS | 60 minut CPU, jeden epizod z zakłóceniem i zmianą nastaw |
| `2ad217...` · `la-bloomml-mpc-scenario-matrix-20261104-v1.json` | 33/33 PASS jednostkowe/integracyjne | 11 scenariuszy × 2 seedy × 3 polityki = 66 uruchomień; syntetyczna **kwalifikacja FAIL** |
| `9258ae41...` | **UNKNOWN — brak potwierdzonego retestu** | Nowe hard-stop/awaryjne interlocki nie są automatycznie objęte poprzednimi PASS |

### Jedyny godzinny epizod, CPU, na `02ac7e...`

| Metryka | MPC | Threshold | Hysteresis |
| --- | ---: | ---: | ---: |
| T MAE, °C | **1.1168** | 6.3600 | 5.2187 |
| RH MAE, punkty proc. | **5.7107** | 21.6017 | 24.1824 |
| CO₂ MAE, ppm | **11.3987** | 140.0813 | 137.7907 |
| Energia *nameplate proxy*, Wh | **223.60** | 380.16 | 379.59 |
| Liczba przełączeń | **29** | 355 | 349 |
| Naruszenia hard envelope, próbkowane | **0** | 1 | 22 |
| Quality gate epizodu | PASS | FAIL | FAIL |

**Kontrprzykłady:** macierz przeciwnych warunków dała MPC **3/22 PASS, 19/22 FAIL**, threshold i hysteresis **0/22 PASS**. W obu startach gorąco/wilgotno threshold miał *niższy błąd temperatury* niż MPC. Zero próbkowanych naruszeń hard-envelope w 22 próbach MPC nie kasuje niepowodzeń śledzenia celu. Faulty, stale readings, nieosiągalne nastawy i warm-up są osobnymi przyczynami do zdiagnozowania, nie ukrywania.

### Statusy, których nie wolno pomylić

- `PASS`: ukończony, zweryfikowany test *w określonym zakresie i SHA*.
- `FAIL`: nieudane wykonanie, naruszenie safety, fallback lub przekroczona tolerancja według zdefiniowanego gate.
- `BLOCKED`: brak warunku wstępnego (np. zatwierdzenia fizycznego sterowania).
- `UNKNOWN`: brak dowodu, SHA, kalibracji albo brak weryfikacji po zmianie.
- **MPC software path:** działa w określonych epizodach; **adversarial quality FAIL**; **ostatni safety retest UNKNOWN**.
- **Empiryczna wierność fizyczna growboxa:** **UNKNOWN**; **wdrożenie na hardware:** **BLOCKED**.

Engineering gate obecnego laboratorium: T ±2°C, RH ±10 pp **lub** VPD ±0.3 kPa, CO₂ ±200 ppm, PPFD ±150 µmol/m²/s. Nie więcej niż 20% próbek poza tolerancją i ani jednego safety violation / fallback. Trzeba raportować osobno warm-up i osiągalność, nie rozluźniać progów po fakcie. 1% nie jest uniwersalną tolerancją.

## 3. Niepewności, znany dług i brakujące dane

**P0 kontroler:** 19/22 niezaliczonych przypadków trudnych; brak osiągalnych obszarów nastaw, osobnego warm-up scorecard, pewnego fallback pod awarie, robust disturbance/uncertainty, szerokich wielodobowych trajektorii; deadline search nie jest twardym realtime. Model aktuatorów ma slew/dwell, ale brak zmierzonych opóźnień dynamiki.

**P0 fizyka:** 0D, założone parametry powierzchni/roślin, niepełny bilans entalpii otwartego układu i niezależny ledger strumieni CO₂, niezbadane krzywe wentylatora/filtra, wilgotności i chłodnicy. DLI reset/photoperiod jest heurystyczny. Weryfikacja bilansu wody nie zastępuje dowodu fizycznego.

**P1 porównania:** energia to proxy z tabliczek znamionowych, nie pomiar poboru; testy baseline nie zawierają jeszcze właściwego PID; potrzeba kilku seedów, niezależnych warunków i niepowodzeń MPC względem baseline z zachowanymi negatywnymi wynikami.

**P1 dane:** autentyczne zewnętrzne NUTRICONTROL (3 pliki / 319 008 rekordów) i WUR Kaggle mirror (6 szklarni / 286 854 pomiarów klimatu) są **innymi szklarniami**. WUR T60 model trendu przegrywa z persistence w 6/6. Oryginalny 4TU 7z i semantyka PAR/aktorów nadal niezweryfikowane. Nie ma własnych długich logów z growboxa.

**Przed ewentualnym sprzętem:** niezależne czujniki i metadane bias/czas/położenie T/RH/CO₂; rzeczywisty przepływ wyciągu przy ΔP/filtrze, wattmetry grzałki/chłodzenia/lamp/wentylatorów, wydajność wilgotności i skroplin, przepływ dozowania CO₂, PPFD/DLI i widmo, geometria i zdolność cieplna, dynamika aktuatorów, potwierdzone *applied* zamiast *requested*, awarie i ACK, pogoda zewnętrzna, rzeczywiste przebiegi wielu dni i split chronologiczny. Bez tego nie ma kwalifikacji fizycznej ani bezpieczeństwa.

## 4. Dokładna kolejność wznowienia

1. **Preflight exact SHA:** `AGENTS.md`, sprawdzenie research HEAD / `main` / `agent-control`, odpowiednie dowody terminalne; nie uruchamiaj GitHub Actions, Codex ani Work. Local Agent (jeśli potrzebny) tylko read-only research clone/test; jego kontrakt z `AGENTS.md` wymaga `work_branch=main` i `resources=[]` — nie wolno użyć go do zapisu na `main`.
2. **P0:** skupiony retest ostatnich hard-stop i interlocków na `9258ae41...` / aktualnym exact code SHA; nowy wynik ma podać source SHA, suite, log, PASS/FAIL. Nie przepisuj historycznych 33/33 jako nowego wyniku.
3. **P0:** analiza 19/22 porażek: per przyczyna, reachable set, warm-up versus steady state, żądanie/applied, odchylenia i nieosiągalne cele. Popraw minimalnie politykę/search i udowodnij zmiany porównaniem na tych samych seedach i identycznej macierzy.
4. **P1:** kosztowo kontrolowane 2 h → 6 h → 24 h/72 h CPU E2E; start od cold/hot, setpoint/ambient/fault/stale, niezależne tolerancje, zero zamaskowanych awarii; dopiero potem multi-seed.
5. **P1:** benchmark identycznego zadania CPU↔GPU na tym samym hoście, porównanie wyników/perf, projekt runnera MPC/physics batched; **dopiero po bramie lokalnej Kaggle 2×T4**, wg [MPC roadmap](../simulator/MPC_ROADMAP.md) i [Kaggle flow](../simulator/KAGGLE_EXECUTION.md). Tysiące konfiguracji × seedy oraz rozkład porażek, nie tylko średnia.
6. **P2:** zewnętrzna walidacja/realne dane i shadow faktycznych opóźnień (bez podłączenia firmware w tym sprincie). Jeśli brak danych, status fidelity pozostaje UNKNOWN, deployment BLOCKED.

### Polecenia kontrolne (repo root; po każdym raportuj exact SHA)

```bash
PYTHONPATH=. python -m unittest -q tests.test_mpc_devices tests.test_mpc_optimizer_mock tests.test_mpc_baselines tests.test_mpc_device_responses tests.test_mpc_physics_integration tests.test_mpc_lab_observability tests.test_mpc_shadow_contract tests.test_mpc_scenario_catalog
PYTHONPATH=. python -m tools.ml.mpc.cli --hours 1 --step-s 60 --seed 1729 --output-dir build/mpc-hourly
PYTHONPATH=. python -m tools.ml.mpc.scenario_matrix --steps 12 --step-s 60 --seeds 1729 1730 --output-dir build/mpc-matrix
```

**UWAGA:** CLI i matrix mogą zwrócić exit 2, jeśli gate jakości FAIL mimo kompletnego poprawnego przebiegu. Zachowaj manifest i ślad; odróżnij FAIL eksperymentu od wyjątku technicznego. Nie uruchamiaj ciężkich obliczeń bez hipotezy i budżetu.

**Decyzja końcowa checkpointu:** **READY TO RESUME OFFLINE CODE**, **MPC ROBUSTNESS = FAIL**, **PHYSICAL FIDELITY = UNKNOWN**, **KAGGLE MPC SWEEP = BLOCKED pending local gates**, **PHYSICAL CONTROL = BLOCKED**.
