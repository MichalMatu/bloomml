# BloomML — COMPLETE RESEARCH HANDOFF / checkpoint 2026-11-04

**Czytaj od tego pliku w następnej rozmowie.** Jeden punkt wejścia statusu: [RESEARCH_STATUS.md](../simulator/RESEARCH_STATUS.md). Jakość/architektura/obserwowalność: [QUALITY_AND_OBSERVABILITY.md](../simulator/QUALITY_AND_OBSERVABILITY.md). Obsługa Kaggle/Local Agent: [KAGGLE_EXECUTION.md](../simulator/KAGGLE_EXECUTION.md). Historia eksperymentu: [R4 dual-T4](2026-11-04-r4-dual-t4-stress.md).

## Tożsamość, tryb i granice

- Repo **`MichalMatu/bloomml`**, roboczy branch **`research/simulator-fidelity-kaggle-20261103`**.
- Autorytet to **aktualny HEAD z GitHub odczytany w nowej rozmowie**, nie historyczny SHA checkpointu.
- Produkcyjny `main` ostatnio zweryfikowany jako `51c90cd3290efde230f0c929e93aaf620e0de106`; przed następnym zadaniem odczytaj go na nowo. **Zero zmian firmware/Growclip/main** podczas tego badania.
- Wykonanie: GitHub Connector + ChatGPT Linux Sandbox; Local Agent na MacBooku tylko dla poleceń/obserwacji hosta i uwierzytelnionego Kaggle CLI. Bez Codex, ChatGPT Work i bez GitHub Actions (nie sprawdzać ich statusów).
- `AGENTS.md` to granica bezpieczeństwa i wymaga przed zadaniami odczytu `main`, `agent-control:.agent/status/daemon.json`, `docs/README.md`, `docs/CURRENT_STATUS.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md`, `docs/PROJECT_LAYOUT.md` i dokumentacji badanego subsystemu. Local Agent tasks muszą używać bindingu repo, `work_branch="main"`, `resources=[]`; **nie** edytować `main`, jeśli zadanie dotyczy badań (Mac tylko odczyt).
- Stare `AGENTS.md` i `docs/ARCHITECTURE.md` zawierają produkcyjne reguły — respektować je, ale nie traktować UI/ESP-IDF jako planu symulatora.

## Co zrobiliśmy — dowody, nie deklaracje

1. **R0:** referencyjny CPU model termodynamiczny 0D, mierzalne inwarianty i deterministyczne golden trajectories. Bilans para/liquid, energia lamp, 4 węzły T, PPFD/VPD/CO₂, oddzielne circulation i exhaust. Parametry nadal priory/założenia.
2. **R1–R2:** Kaggle 2× NVIDIA T4 działa. Potwierdzony fizyczny solver Torch CUDA FP64 na dwóch oddzielnych kartach, CPU↔CUDA pilot parity, bez cichego fallbacku.
3. **R4 V1 (SHA `c9e5d2e37e02aa345d25606120416b357175d296`):** prawdziwe dwa T4, 8192 komór × 180 ticków (10s), **24 424 020 substeps**. Zauważono niefizyczne RH **0,0133%** spowodowane usuwaniem wody ze stałą szybkością przez osuszacz.
4. **R4 V2 po naprawie (SHA `05953e0139cfc31c7ba1fde6ba181d6a90bb9fb3`):** identyczne obciążenie, GPU0 ok. **168,69 s**, GPU1 **177,88 s**, `COMPLETE`; min RH **6,65%**, 0 przypadków RH<5%; T 9,65–34,69°C; 25 z 8192 scenariuszy CO₂>2000 ppm, max ok. 4450 ppm; `|water residual|<3,8e-14 kg`; max błędu pilot CPU↔GPU ~5,7e-14. Manifest `r4_stress_manifest.json` pobrany przez Mac Local Agent i zweryfikowany.
5. **R4 model:** sprzężona para↔mgła z ciepłem utajonym, czasowo ograniczone odparowanie mgły, opcjonalny Q(ΔP) tylko przy danych krzywej wentylatora, prostszy sensible heat dry-air + vapor, osuszacz ograniczony punktem rosy chłodnicy.
6. **R2/R4 quality:** **51/51** uprzednich testów matematycznych CPU/Torch w sandboxie, plus **12** nowych testów kontroli manifestów i import-boundaries — łącznie **63/63 lokalnie PASS**; `python -m compileall` PASS. Audyt AST dla 6 modułów `tools/ml/physics/`: brak zależności od produkcyjnego firmware/serial/network. `growbox_dynamics.py` ok. 599 linii wymaga przeglądu dekompozycji, `growbox_tensor.py` posiada równania paralelne, więc parity to obowiązkowa brama.
7. **Observability:** dodano `tools/ml/research_evidence.py`, read-only fail-closed audit schema/source SHA, dwóch CUDA, sum scenariuszy, parity, bilansu masy i zakresów T/RH/CO₂. Sygnalizuje RH<5% i CO₂>2000 ppm jako warnings; **nigdy** nie potwierdza walidacji empirycznej.
8. **Cleanup dokumentacji:** zamiast nakładających się opisów R1/R2/R4 w README i roadmapie utworzono pojedynczy `docs/simulator/RESEARCH_STATUS.md`; opis modułów i quality w `QUALITY_AND_OBSERVABILITY.md`; skrócono `docs/simulator/README.md` oraz `KAGGLE_EXECUTION.md`, produkcyjne `docs/PROJECT_ROADMAP.md` / `CURRENT_STATUS.md` mają jedynie krótkie przekierowanie do badań. **Nie usuwano kodu ani historycznych dowodów**.

## Dodatkowa kontrola dokładnego checkoutu — Mac Local Agent

Po zapisaniu kodu i dokumentów Local Agent sklonował branch w izolowanym katalogu, zweryfikował **checkout SHA `4a0d66a4322fa98576b41e80afb57fe5e137288f`**, kompilację `research_evidence.py` i jego testów oraz brak niedozwolonych zależności w 6 modułach fizyki — **PASS**. Uruchomił nowy walidator na **oryginalnym prywatnym R4 V2 manifeście** pobranym przez Kaggle CLI: `NUMERICAL_EVIDENCE_PASS`, 8192 scenariusze, 2 CUDA, 1 474 560 ticków, 2 ostrzeżenia dotyczące CO₂>2000ppm (13 na GPU0, 12 na GPU1), `real_world_fidelity=UNKNOWN`.

**Uczciwe ograniczenie:** `pytest` nie był zainstalowany w środowisku Python Mac Local Agenta, dlatego **nie** uruchomiono tam pełnego zestawu. 63/63 lokalnych testów w ChatGPT Linux Sandbox dotyczyło odpowiedniego snapshotu kodu R4 + nowego audytu; exact-checkout Mac potwierdził składnię i wynik audytu, ale nie jest alternatywnym uruchomieniem 63 testów. Nie instalowano pakietów do PlatformIO.

Ślad: `agent-control:.agent/results/la-bloomml-research-checkpoint-evidence-20261104-v1.json` (done/exit 0, `BLOOMML_CLEAN_CHECKPOINT_PREFLIGHT|PASS`). Kolejne commity dokumentacji nie zmieniają równoważności tego raportu z ocenionym kodem. Przed nowymi zmianami należy sprawdzić świeży HEAD.

**Spójność nawigacji:** sprawdzono na GitHub **32 lokalne odnośniki Markdown** w ośmiu aktywnych dokumentach (README/status/roadmap/quality/checkpoint/Kaggle); **0 brakujących celów**. Zachowano historyczne materiały bez przepisywania lub usuwania z repozytorium.

## Czego nie mamy (STOP dla zbyt optymistycznych wniosków)

- **Brak realnych logów czujników użytkownika.** Nie ma kalibracji ani testu modelu na oddzielnych dniach obserwacji. Wyjścia z symulatora to `synthetic_uncalibrated`, a nie pomiary.
- Model 0D nie rozwiązuje 3D przepływów ani pola lampy/gradientów między roślinami. Bilans pełnej otwartej entalpii wilgotnego powietrza i suchego powietrza jest uproszczony; prior cewki osuszacza 12°C i kinetyka mgły 60s **nie są empiryczne**.
- 25 przypadków wysokiego CO₂ może być konsekwencją zadanych dozowań przy małej wentylacji; nie spełnia to automatycznie celów uprawy ani bezpieczeństwa.
- Zgodność CPU↔GPU to porównanie wybranej małej próbki, nie wszystkich 8192 trajektorii. Brak fair CPU-vs-GPU speedup i kwalifikacji FP32.
- Nie należy trenować sterowania produkcyjnego na samych syntetycznych danych ani deklarować „digital twin validated”.

## Co zrobimy po powrocie — jeden następny sprint

**Bez własnych logów priorytetem jest discovery publicznych datasetów**, nie nowe przebiegi 24 mln kroków:

1. Znaleźć źródła (GitHub, Kaggle, Zenodo, Figshare, szklarniowe prace naukowe) rzeczywistych pomiarów klimatu komór uprawowych/szklarni.
2. Zrobić tabelę **licencja, oryginalne źródło, liczba dni, regularność czasu, UTC/strefa, jednostki, sensowne T/RH/CO₂, PPFD lub W, wykonanie HVAC/fan/mist/light, ustawienia czujników i brak leakage**. Nie pobierać wielkich prywatnych danych bez potrzeby, nie commitować zbiorów do Git.
3. Rozdzielić `measured_external` od `measured_own_growbox` i `synthetic_uncalibrated`. Domain shift jest jawny; eksperyment external tylko do badań transferu i walidacji proxy.
4. Stworzyć mały, licencjonowany importer i test jakości danych; chronologiczny train/test split z held-out dniem/szklarnią i persistence baseline, MAE/RMSE/bias dla horyzontów 1/5/15/60 min.
5. Dopiero po wynikach wybrać prace R3/R4/R6/R7; moduły produkcyjne pozostają poza zakresem.

## Szybki preflight / wznowienie

```bash
# w root checkoutu branch badawczego:
PYTHONPATH=. python -m pytest -q tests/test_growbox_thermodynamics.py tests/test_growbox_measurements.py tests/test_growbox_edge_checkpoint.py tests/test_growbox_randomized_edge_cases.py tests/test_growbox_sweep.py tests/test_growbox_tensor.py tests/test_kaggle_gpu_r2.py tests/test_growbox_r4.py tests/test_growbox_r4_convergence.py tests/test_kaggle_r4_stress.py tests/test_research_evidence.py
# po pobraniu *jedynie* zaufanego manifestu:
PYTHONPATH=. python -m tools.ml.research_evidence /path/to/r4_stress_manifest.json --root .
```

**Dowód runu z Local Agent:** `agent-control:.agent/results/la-bloomml-r4-corrected-kaggle-verify-20261104-v2.json`, exit 0 / `R4_V2_DUAL_T4|PASS_NUMERICAL_ONLY`. Kaggle: https://www.kaggle.com/code/meeehooow/bloomml-r4-physics-stress .

**Status checkpointu:** `RESEARCH_DOCUMENTATION_CONSOLIDATED`; `NUMERICAL_TESTS_PASS`; `DUAL_T4_HEAVY_STRESS_VERIFIED`; `EMPIRICAL_VALIDATION_BLOCKED_NO_LOGS`; `PUBLIC_DATASET_DISCOVERY_NEXT`; `PRODUCTION_UNCHANGED`. Wszelkie modyfikacje po tym checkpointcie wymagają aktualizacji śladu HEAD/quality.
