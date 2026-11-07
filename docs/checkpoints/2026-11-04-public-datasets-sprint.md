# BloomML — public greenhouse datasets: sprint handoff 2026-11-04

> **Kontynuacja wykonana:** pełny odczyt kopii WUR 2019 (6 szklarni / CO₂/PAR) i pierwszy test transferu zapisano w [checkpoint WUR](2026-11-04-wur-2019-dataset-benchmark.md). Ten plik zachowuje wcześniejszy stan NUTRICONTROL; stary punkt „WUR następny” jest historyczny.

**Wykonano wyszukiwanie publicznych danych, audyt rzeczywistych plików i mierzalny benchmark.** Pełen katalog z licencjami i wynikami: [PUBLIC_DATASETS.md](../simulator/PUBLIC_DATASETS.md). Aktywny zakres: [RESEARCH_STATUS.md](../simulator/RESEARCH_STATUS.md). Badania na branchu `research/simulator-fidelity-kaggle-20261103`, wyłącznie offline. Produkcyjny `main`, firmware, Growclip oraz Rule/OutputSupervisor **niezmienione**.

## Wynik

1. Zidentyfikowano źródła: **NUTRICONTROL CC BY 4.0** (GitHub + DOI `10.5281/zenodo.21037154`), **WUR Autonomous Greenhouse Challenge 2019 CC0** (DOI `10.4121/uuid:88d22c60-21b3-4ea8-90db-20249a5be2a7`), **Korea 2026 CC BY 4.0** (Mendeley `10.17632/pf24hxwygf.1`), BYU GETS, Shirvan, Belgia, Algieria. Karta syntetycznych/pochodnych danych Pomona — nie dopuszczona jako evidence.
2. **Pobrano trzy prawdziwe pliki CSV** NUTRICONTROL: Álava 80 548, Cartagena C0 69 792, Navarra C0 168 668; **319 008 wierszy**; wszystkie czasy rosnące, nie wykryto duplikatów w próbach; wykryto duże braki kolumn RH/aktorów, kolumna `dateTime` nie ma potwierdzonej strefy czasowej. Wszystkie sumy SHA-256 oraz nazwy są w [raporcie](../simulator/PUBLIC_DATASETS.md).
3. **Źródło przypięte:** `SmartGreenHouses/NUTRICONTROL_datasets` GitHub commit `6dece0d2c7483982610e4d2de225db3c4649b935`. Zweryfikowano także oficjalny tag wydania `V1.0.0` (SHA `f875925e7c1933f978fde15775a8988c26fcbe23`): trzy sprawdzane pliki mają te same Git blob SHA w tagu i odczytanym main, więc nie są nowszą, niesprawdzoną zamianą danych. Licencja CC BY 4.0 potwierdzona w opisie wersji Zenodo (przy wykorzystaniu podać DOI i autorów).
4. **Kod przygotowany:** `tools/ml/external_greenhouse_benchmark.py` to osobny stdlib-only read-only importer/benchmark T/RH. Nie dziedziczy źródeł czasu, nie zakłada UTC, nie interpoluje, odrzuca błędne schematy/hash/timestampy i rozdziela `measured_external_processed_not_own_growbox` od `measured_own_growbox`. Stan urządzeń nie jest traktowany jako potwierdzony fizyczny przepływ.
5. **Testy:** `tests/test_external_greenhouse_benchmark.py` — **7/7 PASS** na Macu; testy braków, outlierów, stref czasu, duplikatów, hashu, trzech mapowań oraz chronologicznego holdoutu. Osobno uruchomiono importer na trzech rzeczywistych CSV, pin SHA256 PASS. Brak zależności Python dodanych do PlatformIO. Nie twierdzić, że ponownie uruchomiono wszystkie 63 wcześniejsze testy fizyki — to inne, historyczne evidence.
6. **Benchmark odłożonych dni:** ostatnie 20% unikalnych dni per komora, persistence `y(t+h)=y(t)` bez fit/treningu/forward fill, exact clock 15/60min; `1/5min` unsupported przy 15min. Álava T MAE 0.384°C / 1.169°C, RH 1.161 / 3.619 pp; Cartagena T 0.340 / 1.206°C, RH 1.215 / 3.718 pp (ale **39.6%** kompletnych RH); Navarra T 0.438 / 1.258°C, RH 1.895 / 4.979 pp (RH tylko **74.2%** complete). Szczegółowe liczności i RMSE w logu Local Agent.
7. **Obserwowalność:** macierz wartości/provenance i liczności, konkretne SHA256, clock basis, source tag/commit, data coverage, liczba odłożonych dni, liczność par per target/horizon, osobne MAE/RMSE/bias. Publiczne pliki usunięto z ephemeral katalogu Mac po audycie; nie commitowano danych ani sekretów.

## Evidence

- `agent-control:.agent/results/la-bloomml-public-greenhouse-dataset-audit-20261104-v1.json` — pierwszy skan trzech rzeczywistych CSV (status done/exit 0).
- `agent-control:.agent/results/la-bloomml-public-datasets-baseline-20261104-v1.json` — 7/7 unit tests plus trzy rzeczywiste baseline, SHA pinned (status done/exit 0).
- Zewnętrzny najważniejszy DOI dla CO₂/PAR: [Wageningen AGC 2019](https://doi.org/10.4121/uuid:88d22c60-21b3-4ea8-90db-20249a5be2a7). Według źródła oficjalne archiwum `AutonomousGreenhouseChallenge_edition2.7z` 8 418 715 B i MD5 `2a0c7f3332881caef54ca8f4dc60c9a3`; zawartość w tym sprincie **jeszcze nie sprawdzona**, więc dopuszczenie WUR do konkretnych eksperymentów nadal otwarte.
- Do fizycznej weryfikacji na niezależnej parze zmierzone/symulowane: [Korea — Mendeley](https://data.mendeley.com/datasets/pf24hxwygf/1), 278 dni, 40 032 odczyty co 10min, **tylko opisy i licencja sprawdzone, nie same pliki**.

## Ocena treningu

**Można zacząć eksperymentalne prognozowanie temperatury T** na kompletnych odcinkach zewnętrznych komór (najpierw Álava) pod warunkiem jawnego przypisania domeny i porównania wyników na tych samych dniach z persistence. **Nie można** jeszcze uznać danych za dobrą kalibrację własnego growboxa: inna geometria, rośliny, urządzenia, nieznana strefa zegara, sporo braków RH i sterowania. **Nie można** trenować sterowania urządzeniami na wartościach setpoint jak na fizycznym potwierdzeniu wykonania. CO₂, PPFD/światło i prawdziwa fizyka ruchu powietrza wymagają osobnych datasetów.

## Dokładna kolejność następnego sprintu

1. Otworzyć oficjalny **WUR 2019 CC0, wersja 2** i wyciągnąć mały zakres tabel: CO₂ ppm, T/RH, PPFD/PAR, światło, temperaturę zewn., statusy grzania/wyciągu, czas, komora i brakujące wartości. Zapis źródła + hash, żadnych ogromnych CSV w Git.
2. Zweryfikować, które kolumny to `measured`, `requested`, `realized` i `simulated`; sprawdzić jednostki, interwały, strefę czasową, sensor bias i chronologiczne luki.
3. Rozszerzyć importer *po oględzinach prawdziwego schematu* WUR. Przeprowadzić analogiczne benchmarki i test splitu po **komorze oraz po dniach**.
4. Zrobić pierwszy lekki benchmark modelu T/RH/CO₂ na Kaggle **dopiero po** odrzuceniu kolumn z przeciekiem i zweryfikowaniu kompletnych etykiet; porównywać z realną persistence. Odrębna walidacja fizyki BloomML wymaga wspólnych działań i pogody.
5. Pozyskać dane własnego growboxa, gdy będą dostępne; dopiero one umożliwią R6 real-box calibration.

**Resume:** przed kolejnym kodowaniem odczytaj `AGENTS.md`, `docs/simulator/RESEARCH_STATUS.md`, ten checkpoint, `PUBLIC_DATASETS.md`, aktualny branch HEAD, `main` i `agent-control:.agent/status/daemon.json`. Nie używać Codex/Work/GitHub Actions. Brak prac na produkcji.
