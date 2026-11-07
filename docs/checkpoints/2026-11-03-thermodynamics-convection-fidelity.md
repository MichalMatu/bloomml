# Checkpoint: termodynamika, konwekcja i cyrkulacja — 2026-11-03

**Scope:** offline research-only. No Growclip, firmware, climate-v6 runtime/control or `main` changes.
**Branch:** `research/simulator-fidelity-kaggle-20261103`.
**Status:** `NUMERIC_REGRESSION_PASS / PHYSICAL_FIDELITY_UNVERIFIED / GPU_NOT_RUN / NOT_TRAINING_READY`.

## Moduły i odpowiedzialność

- `tools/ml/physics/growbox_dynamics.py` — jeden właściciel stanu, bilans ciepła, masy H2O, CO2, światło, biologia, źródła zewnętrzne.
- `growbox_psychrometrics.py` — nasycenie, ciśnienie pary, punkt rosy, VPD powietrza i liści, gęstość wilgotnego powietrza.
- `growbox_convection.py` — naturalna i wymuszona konwekcja, empiryczny wpływ cyrkulatora/wyciągu na prędkość lokalną.
- `growbox_measurements.py` — katalog wejść i provenance (rzeczywisty pomiar vs estymata).
- `run_thermodynamic_sweep.py` — odtwarzalne trajektorie CPU v2, przyszły oracle dla GPU.

Obecny `climate_simulator.py` nadal dostarcza dane do istniejącego v6 dataset/teacher. **Nowy fizyczny rdzeń NIE jest jeszcze podłączony do treningu v6 ani urządzenia.**

## Co naprawiono i dlaczego

1. **Cyrkulacja nie jest wentylacją zewnętrzną.** Parametr `circulation_fan` wpływa na konwekcyjny transport między liśćmi, powietrzem, ścianami i podłożem, masowy transport przy powierzchniach oraz ciepło silnika. Nie zwiększa sztucznie ACH/odpływu CO2. `exhaust_fan` zachowuje własną rolę strumienia zewnętrznego.
2. **Konwekcja naturalna i wymuszona.** Zamiast niezmiennego współczynnika liść–powietrze i zerowego wpływu ruchu na ściany/podłoże zastosowano porównywalne skale h(U, ΔT, L). Wymuszenie: Nu = 0,664 Re^0,5 Pr^(1/3) (Re ≤ 5e5), korelacja średnia turbulentna dla większego Re; naturalna: h ~ 1,31 (|ΔT|/L)^(1/3), łączone przez normę sześcienną. To **empiryczne** przybliżenia transportu, nie CFD ani potwierdzona geometria wewnętrzna. Współczynniki bazowe UA i masowe muszą zostać skalibrowane.
3. **Prędkość lokalna przy liściach.** Zmierzona wartość wygrywa nad estymatą, a wynik jest oznaczony `measured` lub `estimated`. Estymata łączy tło, wyciąg i cyrkulację w RMS; wrażliwa na ułożenie wentylatorów i roślin.
4. **Energia izolowanego powietrza.** Zmienna po każdym podkroku masa cieplna ρ(T) kreowała energię w zamkniętym bilansie. Referencyjna bezwładność przy zadanym `reference_pressure_pa` jest utrzymywana w jednym runie; transport ciepła zewnętrznego pozostaje osobnym strumieniem. Nie oznacza to pełnego bilansu entalpii suchego/wilgotnego powietrza przy otwartym przepływie.
5. **Stabilność i atomowość.** Limit podkroków obejmuje maksymalne sprzężenia konwekcyjne po stronie powietrza, ścian, liści i podłoża. Wyjątek podczas podkroku nie publikuje częściowo zaktualizowanego stanu.
6. **Golden run v2.** Do scenariuszy dodano cyrkulację i zapis diagnozy prędkości w pobliżu liści. Nowa seria *nie jest liczbowo porównywalna* z v1 przy tym samym seedzie (zmieniła się liczba losowań).

## Dowody w lokalnym środowisku testowym

```bash
PYTHONPATH=. python -m pytest -q \
  tests/test_growbox_thermodynamics.py \
  tests/test_growbox_measurements.py \
  tests/test_growbox_sweep.py \
  tests/test_growbox_edge_checkpoint.py \
  tests/test_growbox_randomized_edge_cases.py
```

**Wynik lokalny: 33/33 PASS** (Python systemowy dostępny w Linux Sandbox, nie pełna referencyjna paczka Python 3.11).
Badane: kondensacja, wilgoć, masa i energia jawna+utajona w komorze izolowanej, temperatura liści/ścian/podłoża, dwa rodzaje wentylatorów, dane zmierzone vs szacowane, błąd wejścia, rollback błędnego kroku, zbieżność 1 s / 10 s i **100 syntetycznych scenariuszy × 12 kroków** z ustalonym seedem.

```bash
PYTHONPATH=. python -m tools.ml.physics.run_thermodynamic_sweep \
  --scenarios 40 --steps 720 --seed 1729 \
  --summary build/thermodynamic-golden-convection-v2.json
```

**Wynik lokalny: 28 800 kroków**, maks. residual bilansu masy wody `1.7319479184152442e-13 kg`; SHA256 kanonicznych NDJSON `3ce606f897d05c669393d8ad88fd9e7f5a2ad50c0129885d14b23eba7e19b8d9`.

## Otwarte zależności: priorytety i pomiary

| P | Zjawisko nadal uproszczone | Co mierzyć / zrobić |
|---|---|---|
| P0 | **Rozkład przestrzenny:** jedna doskonale wymieszana strefa nie odtwarza zastoisk, skrótów między wlotem a wyciągiem, gradientów pionowych temperatury/RH/CO2 ani lokalnego VPD | T/RH/CO2 na minimum 2–3 wysokościach, prędkość powietrza w koronie i przy wlocie; geometria i orientacja wentylatorów; następnie 2+ strefy lub zweryfikowany CFD→ROM |
| P0 | **Q(fan, ΔP):** brak krzywej wentylatora, oporu filtra, przecieków zależnych od ciśnienia i bezwładności silnika | Anemometr lub przepływomierz, pomiar różnicy ciśnień, różne obroty, filtr czysty/brudny |
| P0 | **Entalpia i masa suchego powietrza przy wentylacji:** przepływ masy i bilans cieplny wilgotnego powietrza w warunkach wysokiej ΔT/ΔRH jeszcze uproszczony | Dodać bilans suchego powietrza i pełnej entalpii napływu/odpływu; testy energii dla otwartego układu |
| P1 | **Radiacja lamp/otoczenia:** uproszczony rozkład mocy, brak view-factors, widma, IR długofalowego i temperatury drivera lampy | PPFD na siatce punktów, W rzeczywiste lampy, termografia liści/ścian, położenie lampy, charakterystyka widmowa |
| P1 | **Liście i transpiracja:** proxy PPFD/VPD/woda, bez LAI i oporu szparkowego oraz zależności od gatunku/fazy | Temperatura liści, LAI, masa wody i biomasy, PPFD, czas i poziom stresu |
| P1 | **Chłodzenie/osuszanie:** uproszczone dane punktu rosy cewki, kondensacji i ciepła silników; brak rzeczywistych charakterystyk vs T/RH/przepływ | Temperatura wymiennika, rzeczywista moc W, masa kondensatu, przepływ |
| P1 | **Substrat:** jeden zasób wilgoci, bez gradientu i krzywej retencji; ograniczona wymiana ciepła z donicą | Czujniki na głębokościach, masa donicy, charakterystyka podłoża, odwodnienie |
| P1 | **Szron poniżej 0°C:** psychrometria nad ciekłą wodą, brak osobnej fazy lodowej | Pomiar powierzchni i model przejść fazowych przy takich scenariuszach |
| P1 | **Sensory:** brak modelu czasu odpowiedzi, histerezy, dryftu oraz korelacji błędów | ID, położenie, age, valid, kalibracja i zsynchronizowane logi |
| P2 | **Metabolizm CO2 roślin:** uproszczona fotosynteza/oddychanie, bez zmian biomasy i obsady | Faza rozwoju, obsada, bilans CO2/wody w długim horyzoncie |

Nie dodawać pozornie precyzyjnych parametrów bez możliwości kalibracji. Surowe logi z ESP32 to pomiary i komendy, niekoniecznie dowód fizycznego wykonania przekaźników.

## Plan dalszy i wymóg GPU

1. Zweryfikować zgodność importu rzeczywistych pomiarów z obserwacjami: T/RH/CO2, W lampy, aktywne urządzenia, PPFD, temperatury ścian/liści, przepływ i ciśnienie.
2. Rozdzielić dopasowanie parametrów od walidacji na **innych dniach**. Porównać z prostym baseline persistence i raportować MAE/RMSE/bias po 1, 5, 15, 60 minutach; ustalić dopiero wtedy kryteria akceptacji.
3. Gdy będą fizyczne dane gradientów, rozbudować do modelu wielostrefowego, zachowując zerowe pozorne strumienie/ścisłe bilanse.
4. Dopiero po zamknięciu fizyki zrobić batched backend Torch/JAX oraz CPU↔GPU parity, następnie sharding niezależnych przebiegów na **dwa oddzielne T4 16 GB** w Kaggle. To **nie jest** wspólne 32 GB VRAM.
5. Model uczony zostaje w offline research. ESP32-S3 może dostać później wyłącznie zweryfikowanego małego studenta jako shadow; Rule / OutputSupervisor pozostają autorytetem.

**Nie uruchamiano Kaggle GPU, Local Agenta na sprzęcie, firmware ani GitHub Actions.** Dokument nie jest certyfikatem odwzorowania rzeczywistej fizyki growboxa.
