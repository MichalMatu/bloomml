# BloomML R2 — tensorowy solver termodynamiczny: sprint parity

Data: 2026-11-03. Repo: `MichalMatu/bloomml`. Branch: `research/simulator-fidelity-kaggle-20261103`.
Zakres: **wyłącznie offline research**. `main`, firmware, Growclip, Rule i OutputSupervisor bez zmian.

## Stan po sprincie

- Dodano `tools/ml/physics/growbox_tensor.py`: PyTorch `float64/float32` vectorized physics dla niezależnych, heterogenicznych scenariuszy. Na urządzeniu liczone są wszystkie składowe stanu (cztery temperatury, para wodna, CO₂, bilans wody, transpiracja, kondensacja, mgła, konwekcja, DLI), a nie testowa macierz CUDA. CPU scalar pozostaje wzorcem.
- Każdy scenariusz zachowuje własne `dt/substeps` z modelu referencyjnego; maskowanie aktywnych elementów umożliwia wspólne obliczenia tensorowe bez zmiany metody całkowania.
- Dodano `tools/ml/kaggle_gpu_r2_runner.py`: oddzielne zadania dla `cuda:0` i `cuda:1`, kontrola zgodności z CPU dla małych podgrup, mierzone wall-time i peak CUDA memory, manifest z konkretnym SHA. Żaden fallback CPU nie jest akceptowany jako wynik CUDA.
- Dodano `notebooks/BloomML_Kaggle_GPU_R2.ipynb`: osobny prywatny notebook do weryfikacji obliczeń na rzeczywistych 2× T4.
- Dodano `tests/test_growbox_tensor.py`, `tests/test_kaggle_gpu_r2.py`.

## Wyniki i zastrzeżenia

- **41/41 lokalnych testów PASS w ChatGPT Linux Sandbox** (33 istniejące numeryczne testy referencji + 8 nowych testów tensorów i bramek).
- Nowy backend na CPU/PyTorch odtwarza trajektorie skalara `float64` z niemal maszynową zgodnością w typowych scenariuszach, także zróżnicowane parametry i ciśnienia, przepływy, ogrzewanie, oświetlenie, wilgoć i CO₂.
- `float32` ma osobną, mniej rygorystyczną bramkę błędów; nie jest automatycznie uznany za zgodny z `float64`.
- **PRAWDZIWE GPU PHYSICS PASS (Kaggle T4 ×2, Version #1):** prywatny notebook `meeehooow/bloomml-gpu-physics-r2` ukończony jako `KernelWorkerStatus.COMPLETE`. Mac Local Agent pobrał tylko `r2_gpu_physics_manifest.json`, sprawdził `source_commit=84ee2245550b095422a9e5dd6c74834bd1ecc8f4`, `gpu_physics_ready=true`, `simulation_backend=torch_cuda_batched_euler_v1`, dwa urządzenia CUDA, bilans wody i flagi `calibrated=false`, `training_ready=false`. R1 (osobny starszy notebook) miał dwie karty CUDA i 480 kroków CPU; pełnego manifestu R1 nie udało się pobrać przez timeout.
- **Silna kondensacja/mgła:** w 0D solverze referencyjnym stwierdzono niestabilność/wrażliwość na błędy zaokrągleń przy długim przebiegu z gorącym nasyconym powietrzem i znacznie chłodniejszymi ścianami/liśćmi. Krótkie przebiegi mają zgodność, długie wymagają badania zbieżności kroku i poprawy fizyki fazowej w R4; bez sztucznego zwiększania tolerancji parity.
- **R2 PILOT PASS, kwalifikacja ekstremów OTWARTA:** obie T4 policzyły realne równania fizyki, porównanie CPU↔CUDA (po 8 scenariuszy pilotażowych na kartę) oraz bilans wody przeszły. Wciąż otwarte są testy długich przebiegów/ekstremalnej kondensacji, większe parytety FP32/FP64 i uczciwy benchmark CPU↔GPU na takim samym hostcie. Nie jest to jeszcze skalibrowany cyfrowy bliźniak.

## Faktyczne metryki rzeczywistej sesji Kaggle

| GPU | Zadania | Kroki/scenariusz | CUDA batch wall-time | Peak PyTorch CUDA allocated | Pilot CPU↔CUDA max absolute error |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cuda:0` Tesla T4 | 256 | 8 | 0.986191851 s | 388608 B | 4.336808689942018e-19 |
| `cuda:1` Tesla T4 | 256 | 8 | 0.881886808 s | 388608 B | 4.336808689942018e-19 |

Łącznie 512 scenariuszy i 4096 kroków scenariuszy. Pomiar pamięci dotyczy alokacji tensorów PyTorch, a nie pełnego zużycia VRAM urządzenia. Ten test **nie dowodzi przewagi szybkościowej GPU nad CPU**, bo brak pomiaru CPU na porównywalnym sprzęcie i identycznym zadaniu. Manifest i logi przechowuje prywatny notebook Kaggle: https://www.kaggle.com/code/meeehooow/bloomml-gpu-physics-r2 . Trwała, audytowalna telemetria zadania: `agent-control:.agent/results/la-kaggle-r2-private-submit-20261103-v1.json` oraz `agent-control:.agent/results/la-kaggle-r2-verify-20261103-v1.json` (status `done`, exit 0, `R2_DUAL_CUDA_PHYSICS|PASS`).

## Kolejność dalszych prac

1. **DONE:** prywatny notebook `notebooks/BloomML_Kaggle_GPU_R2.ipynb` uruchomiony przez Mac Local Agent na T4 ×2, manifest pobrany i zweryfikowany przez API.
2. Zmierzyć przepustowość, VRAM i błędy CUDA FP64/FP32; bez benchmarkowego przyspieszenia nie twierdzić, że GPU jest szybsze.
3. Ustabilizować R4: entalpia wilgotnego powietrza przy wentylacji, wentylator Q(ΔP), granice/kinetyka kondensacji i zbieżność dt przy ekstremach.
4. Po realnych logach przejść do R6 (kalibracja i test na innych dniach), potem R7 (duże serie).

Źródło prawdy o całym programie nadal: `docs/checkpoints/2026-11-03-kaggle-bloomml-compute-handoff.md` (R0–R8) oraz `docs/simulator/KAGGLE_EXECUTION.md`.
