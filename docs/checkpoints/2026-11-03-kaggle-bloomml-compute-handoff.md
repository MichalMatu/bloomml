# BloomML — trwały checkpoint: symulator → Kaggle T4×2

**Data:** 2026-11-03. **Repo:** MichalMatu/bloomml. **Branch:** research/simulator-fidelity-kaggle-20261103. **Autorytet źródeł:** GitHub (konkretny commit SHA, nie sam branch). **Środowisko obliczeń intensywnych:** Kaggle, GPU NVIDIA T4 ×2. **Zakres:** wyłącznie offline research; Growclip, firmware i main bez zmian.

## Pierwsze czynności następnej rozmowy (NIE POMIJAĆ)

1. Przeczytaj AGENTS.md, docs/README.md, docs/CURRENT_STATUS.md, docs/ARCHITECTURE.md, docs/PROJECT_ROADMAP.md, ten checkpoint, docs/simulator/KAGGLE_EXECUTION.md oraz docs/checkpoints/2026-11-03-thermodynamics-convection-fidelity.md.
2. GitHub Connector: odśwież HEAD main i research/simulator-fidelity-kaggle-20261103, stan Local Agent na agent-control. Nie zakładaj starego SHA; porównaj diff, testy i aktualne zgłoszenia. Bez Codex, ChatGPT Work i GitHub Actions.
3. Wznów dokładnie ten tor badań, bez przepisywania symulatora od nowa i bez zmian w Growclip/firmware.
4. **Pierwszy konkretny krok:** wykonaj w prywatnym notebooku Kaggle (T4 ×2, Internet ON) notebooks/BloomML_Kaggle_T4x2.ipynb. Zachowaj kaggle_manifest.json i dokładny source_commit. Sprawdź, czy oba GPU faktycznie wykonały CUDA smoke.
5. Dopiero później implementuj wektorowy backend solvera na Torch/JAX i testy parity CPU↔GPU. Aktualna implementacja fizyki jest CPU! Nie wprowadzaj użytkownika w błąd.

## Stały kontrakt BloomML↔Kaggle

- Kod, testy, roadmapa, notebook i checkpointy: GitHub, branch badawczy.
- Notebook Kaggle: pobiera repo/branch przez git, opcjonalnie przypina PINNED_SHA i zawsze odczytuje dokładny commit hash.
- Runner: tools/ml/kaggle_physics_runner.py. Wymaga co najmniej dwóch urządzeń CUDA, wykonuje na każdym z nich arytmetyczny test PyTorch i zapisuje manifest SHA.
- CPU oracle: tools/ml/physics/run_thermodynamic_sweep.py. Generuje złote trajektorie bez zmiany fizyki i zapisuje provenance, digest SHA-256 i residuale. To **nie** jest wykonanie fizyki na GPU.
- Artefakty Kaggle: /kaggle/working/bloomml-run/kaggle_manifest.json oraz cpu_golden.ndjson. Zachowuj prywatnie w outputs/datasets, bo notebook/sesja Kaggle jest nietrwała. Nie publikuj logów rzeczywistego growboxa ani tokenów.
- Kaggle nie udostępnia z automatu stale działającego serwera i nie ma gwarantowanego limitu czasu; wymagane kliknięcie operatora w koncie Kaggle. Żadne automatyczne wysyłanie z GitHub ani z Mac nie zostało uruchomione.
- T4 ×2 = dwa oddzielne urządzenia po 16 GB VRAM, nie jeden GPU z 32 GB. GPU-ensemble realizujemy przez sharding niezależnych scenariuszy, a nie założenie wspólnej pamięci.

## Co faktycznie jest już wykonane

- Model 0D: temperatura powietrza, liści, ścian, podłoża; wilgotność, bilans wody/CO₂, ciśnienie, punkt rosy, VPD powietrza i liści, strumienie lamp/PPFD/DLI, kondensacja i konwekcja naturalna/wymuszona, odrębne wyciąg i wewnętrzna cyrkulacja.
- Poprzedni checkpoint (2026-11-03-thermodynamics-convection-fidelity.md): 33/33 PASS lokalnych testów naukowych, 40 × 720 = 28 800 kroków syntetycznych, maksymalny residual masy wody 1.7319479184152442e-13 kg. To **nie** dowodzi fizycznej zgodności ani działania CUDA.
- Nowy Kaggle runner i notebook zostały zapisane na branchu; w tym checkpointcie realne uruchomienie Kaggle, CUDA parity oraz masowe GPU sweeps są **NOT RUN**.
- Importer SD w BloomML już istnieje, ale dane z poleceń sterownika nie są równoznaczne z fizyczną prawdą o wykonaniu wyjść.
- Produkcyjne sterowanie: Rule + OutputSupervisor pozostają nadrzędne; ML tylko shadow/research.

## Niezmienna roadmapa R0–R8

| Etap | Cel / warunek zamknięcia | Status |
| --- | --- | --- |
| R0 | Rdzeń termodynamiki i konwekcji; testy invariants, ekstremalne przypadki, CPU golden | **NUMERIC PASS, uncalibrated** |
| R1 | Prawdziwe uruchomienie notebooka Kaggle: źródłowy SHA, 2 sprawdzone T4, manifest i CPU baseline | **CODE READY; KAGGLE NOT RUN** |
| R2 | Batched PyTorch/JAX GPU solver, testy CPU↔GPU na skrajnych parametrach, metryki błędu i wydajności; sharding na oba GPU | **OPEN** |
| R3 | Integracja realnych logów SD: T/RH/CO₂, czasy, status/pozycja czujników, fizyczne wykonanie wyjść i jednostki | **PARTIAL** |
| R4 | Fizyka P0: bilans suchego powietrza i entalpii przy wymianie, fan Q(ΔP), opory filtra, kondensacja, transport wody | **OPEN** |
| R5 | Wielostrefowy model ROM/CFD tylko przy udowodnionych gradientach w growboxie; siatka czujników, przepływy, cyrkulacja | **OPEN** |
| R6 | Kalibracja na realnych dniach: test na oddzielnych dniach; MAE/RMSE/bias vs persistence przy 1/5/15/60 min, sensitivity/uncertainty | **OPEN** |
| R7 | Duże eksperymenty na obu T4: identyfikacja parametrów, ensemble, stres, domain randomization; wersjonowany dataset | **BLOCKED by R2/R4/R6** |
| R8 | Mały zweryfikowany model TinyML dla ESP32-S3, wyłącznie shadow do osobnej kwalifikacji | **NOT STARTED** |

**Najbliższy priorytet:** R1 (test istniejącego notebooka na Kaggle) oraz R4 (pełniejsze bilanse fizyczne) w rozdzielnych zmianach. Potem R2 parity. Bez R6 nie wolno uznać syntetycznego datasetu za realistyczny trening produkcyjny.

## Najważniejsze braki matematyczne do zachowania w pamięci projektu

P0: pojedyncza komora doskonale wymieszana nie modeluje rozkładu temperatury/RH/CO₂, warstw przy liściach, stagnacji, bypassu wlot–wylot; brak zmierzonej krzywej wentylatorów i relacji przepływu do ΔP, niepełny bilans entalpii wilgotnego powietrza w wentylacji.

P1: promieniowanie widmowe, kształt/wysokość lampy, mapa PPFD, radiacyjna wymiana liść/ściany, opór szparkowy i LAI roślin, charakterystyki chłodnicy/osuszacza/nawilżacza, woda i gradienty podłoża, szron/zmiana faz poniżej zera, opóźnienia/dryf sensorów.

Zbieraj: wewnętrzne T/RH/CO₂ na różnych wysokościach i zewnętrzne na wlocie, prędkość koło liści, ΔP, przepływ wyciągu, moc lampy W, PPFD w siatce, IR liści/ścian, status fizycznych wyjść, wilgoć donic i masę kondensatu. Każdy odczyt ma ID, czas, valid/age i provenance (measured / estimated). Bez tych danych nie deklarować wysokiej wierności.

## Gdzie wznowić i jak nie zgubić projektu

- Kod fizyki: tools/ml/physics/growbox_dynamics.py, growbox_convection.py, growbox_psychrometrics.py, growbox_measurements.py.
- CPU złoty test: tools/ml/physics/run_thermodynamic_sweep.py.
- Kaggle: notebooks/BloomML_Kaggle_T4x2.ipynb i tools/ml/kaggle_physics_runner.py.
- Operacje Kaggle: docs/simulator/KAGGLE_EXECUTION.md.
- Roadmapa: docs/PROJECT_ROADMAP.md.
- Ten plik: docs/checkpoints/2026-11-03-kaggle-bloomml-compute-handoff.md.

**Checkpoint states:** NUMERIC_REGRESSION_PASS; PHYSICAL_FIDELITY_UNVERIFIED; KAGGLE_GPU_NOT_RUN; GPU_SIMULATOR_BACKEND_OPEN; REAL_DATA_CALIBRATION_OPEN; PRODUCTION_UNCHANGED.
