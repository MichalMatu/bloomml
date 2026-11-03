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
- **Test rzeczywistego solvera na 2× T4 jeszcze NIE WYKONANY w tym sprincie.** R1 przeprowadził CUDA matmul na obu T4 i 480 kroków CPU; Kaggle Version #1 COMPLETE. Manifest R1 przez API nie został pobrany (timeout), więc jego zdalny digest nie jest potwierdzony.
- **Silna kondensacja/mgła:** w 0D solverze referencyjnym stwierdzono niestabilność/wrażliwość na błędy zaokrągleń przy długim przebiegu z gorącym nasyconym powietrzem i znacznie chłodniejszymi ścianami/liśćmi. Krótkie przebiegi mają zgodność, długie wymagają badania zbieżności kroku i poprawy fizyki fazowej w R4; bez sztucznego zwiększania tolerancji parity.
- **Nie ogłaszać pełnego R2 DONE** dopóki oba GPU nie policzą fizyki, nie przejdą parity CPU↔CUDA, walidacji masy i manifestu. Model nie jest skalibrowanym cyfrowym bliźniakiem.

## Kolejność dalszych prac

1. W prywatnej sesji Kaggle T4 ×2 wykonać `notebooks/BloomML_Kaggle_GPU_R2.ipynb` (internet ON) i potwierdzić `r2_gpu_physics_manifest.json` z `gpu_physics_ready: true`, dwoma `devices` i dokładnym source SHA.
2. Zmierzyć przepustowość, VRAM i błędy CUDA FP64/FP32; bez benchmarkowego przyspieszenia nie twierdzić, że GPU jest szybsze.
3. Ustabilizować R4: entalpia wilgotnego powietrza przy wentylacji, wentylator Q(ΔP), granice/kinetyka kondensacji i zbieżność dt przy ekstremach.
4. Po realnych logach przejść do R6 (kalibracja i test na innych dniach), potem R7 (duże serie).

Źródło prawdy o całym programie nadal: `docs/checkpoints/2026-11-03-kaggle-bloomml-compute-handoff.md` (R0–R8) oraz `docs/simulator/KAGGLE_EXECUTION.md`.
