# BloomML — jakość kodu, modułowość i obserwowalność

**Scope:** `tools/ml/physics/`, `tools/ml/kaggle*_runner.py`, `tools/ml/research_evidence.py`, `tests/test_growbox*.py`, `tests/test_kaggle*.py`, `tests/test_research_evidence.py`. Nie dotyczy urządzeń, firmware, UI ani Growclip. Stan 2026-11-04; [jedyny bieżący status](RESEARCH_STATUS.md).

## Granice i odpowiedzialności

| Moduł | Jedyny główny cel | Uwagi jakościowe |
| --- | --- | --- |
| `growbox_psychrometrics.py` (68 linii) | Funkcje SI temperatury, RH, nasycenia, VPD | Czysty, bez wejścia/wyjścia |
| `growbox_convection.py` (91) | Współczynniki konwekcji i prędkość powietrza | Czysty; parametry to założenia |
| `growbox_phase.py` (51) | Konserwatywna przemiana para↔mgła i ciepło utajone | Czysty; stała 60 s niekalibrowana |
| `growbox_dynamics.py` (~599) | Referencyjny stan i skalarne równania 0D | **Ryzyko strukturalne:** duży moduł; rozdzielać dopiero przy zabezpieczeniu testami |
| `growbox_tensor.py` (~333) | Torch / batch / CUDA, niezależne równania dla każdego scenariusza | **Ryzyko driftu:** powielone wzory względem skalara; parity jest konieczny |
| `growbox_measurements.py` (161) | Jednostki i pochodzenie obserwacji / ważność | Nie zamienia szacunków w pomiary |
| `run_thermodynamic_sweep.py` | Deterministyczne scenariusze CPU i hash trajektorii | Dane tylko `synthetic_uncalibrated` |
| `kaggle_gpu_r2_runner.py`, `kaggle_r4_stress_runner.py` | Orkiestracja dwóch GPU, podział partii, wykonanie i manifest | CLI, I/O i stan uruchomienia *poza* domeną fizyki |
| `research_evidence.py` | **Tylko odczyt** manifestu R4 / statyczny audyt importów | Fail-closed; nie importuje Torch i nie czyta tokenów |
| `notebooks/` | Uruchomienie istniejącego runnera na Kaggle | Nie jest źródłem równań ani prawdy pomiarowej |

Test import-boundary na sześciu modułach fizyki blokuje importy z `src/`, `lib/` i z zależności hardware/network; `growbox_dynamics.py` oznacza jako `review_large_file`. Dla dekompozycji tego modułu wymagana jest osobna zmiana z niezmienionymi wynikami kontrolnymi.

## Bramy jakości

1. **Correctness local:** `PYTHONPATH=. python -m pytest -q tests/test_growbox_thermodynamics.py tests/test_growbox_measurements.py tests/test_growbox_edge_checkpoint.py tests/test_growbox_randomized_edge_cases.py tests/test_growbox_sweep.py tests/test_growbox_tensor.py tests/test_kaggle_gpu_r2.py tests/test_growbox_r4.py tests/test_growbox_r4_convergence.py tests/test_kaggle_r4_stress.py tests/test_research_evidence.py` — **63/63 PASS w ChatGPT Linux Sandbox**. W tym 12 nowych przypadków audytu. Ścieżka testowa nie wykonuje CUDA w sandboxie; Torch jest CPU.
2. **Math:** bilans wody `|residual|≤1e-8 kg` (float64), zbieżność kroku 1 s/10 s na 40 scenariuszach, energia przemiany fazowej; testy błędów granicznych i negatywnych danych wejściowych.
3. **CPU↔CUDA:** pilot przez *rzeczywistą* T4 każdej partii; według manifestu V2 maksimum błędu scalar↔tensor ~`5.7e-14`, ale tylko na **24 kontrolnych scenariuszach na GPU**, nie na wszystkich 8192.
4. **CUDA fail-closed:** dokładnie dwie odrębne `cuda:0/1`, bez zastępowania pracy GPU przez CPU. Manifest wymaga 2 urządzeń, identycznej sumy scenariuszy/ticków, znanej schemy i 40-znakowego SHA.
5. **Plausibility:** T w ograniczeniach modelu, RH≤100%, para/kondensat nieujemne, CO₂ skończone, bilansem wody. **ALERT**, nie automatycznie FAIL: RH<5% lub CO₂>2000 ppm; wyniki takich prób należy obejrzeć.
6. **Truth boundary:** wszystkie scenariusze `synthetic_uncalibrated=true`; `physical_validation_with_real_data=false` musi pozostać prawdziwe do chwili importu *niezależnych* pomiarów. Brak benchmarku CPU↔GPU na tym samym hoście nie pozwala obiecywać przyspieszenia.

## Observability — minimalny kontrakt na każdy eksperyment

- `source_commit` z Git; nazwa źródła i wersja notebooka; seed + identyfikator generatora; `dt_s`, `ticks`, `scenarios_total`, struktura kategorii. Porównaj zapisany SHA z tym z pobranego kodu — **sam manifest nie poświadcza autentyczności źródła**.
- Per GPU: `device`, nazwa karty, scenariusze, łączna liczba podkroków, mierzony czas, alokacja PyTorch (nie cała VRAM), mała próbka parity względem CPU.
- Wartości graniczne i rozkład: temperatura, RH, CO₂, masa skroplin/mgły, max water residual, liczba przypadków RH<5%, CO₂>2000ppm. Brakuje jeszcze percentyli i per-kategoria dla surowych trajektorii.
- Dane zewnętrzne/przyszłe: źródło, licencja, `sensor_id`, jednostki, czas UTC, strefa, kompletność, provenance `measured/derived/estimated/synthetic`, zdarzenia lamp/wyjść jako **wykonane**, nie tylko żądane.
- Każdy raport rozróżnia **NUMERICAL_EVIDENCE_PASS**, **PLAUSIBILITY_WARNING**, **EMPIRICAL_FIDELITY_UNKNOWN**; zielony Kaggle status nie dowodzi poprawności fizycznej.

### Odtworzenie audytu manifestu

Pobierz tylko prywatny `r4_stress_manifest.json` przez [instrukcję Kaggle](KAGGLE_EXECUTION.md), potem:

```bash
PYTHONPATH=. python -m tools.ml.research_evidence /path/to/r4_stress_manifest.json --root .
```

Wynik `NUMERICAL_EVIDENCE_PASS` oznacza spójny **zapis** obliczeń. Błędne GPU, brak provenance, niespójne sumy, NaN/Inf, przekroczony bilans, sfałszowana deklaracja kalibracji → `REJECTED` i exit code 2. Ostrzeżenia agronomiczne nie są ukrywane. Tę samą wersję SHA i manifest można sprawdzić w `agent-control` (patrz [checkpoint](../checkpoints/2026-11-04-r4-dual-t4-stress.md)).

## Weryfikacja opublikowanego kodu (Mac, 2026-11-04)

Local Agent sklonował kod z SHA `4a0d66a4322fa98576b41e80afb57fe5e137288f`, sprawdził `compileall` i AST import-boundary (**PASS**), pobrał **oryginalny prywatny** manifest Kaggle R4 V2 i wykonał `python3 -m tools.ml.research_evidence`: **NUMERICAL_EVIDENCE_PASS**, 2 ostrzeżenia CO₂ > 2000 ppm, brak potwierdzenia danych rzeczywistych. Wynik: `agent-control:.agent/results/la-bloomml-research-checkpoint-evidence-20261104-v1.json` (done, exit 0).

Mac `pytest` był **niedostępny** — 63/63 testów uruchomiono w Linux Sandbox na snapshotcie R4 z nowym modułem evidence; nie należy raportować tych testów jako wykonanych na Macu lub jako nowego runu Kaggle. Instalacje w PlatformIO nie były wykonywane.

## Publiczne pomiary jako oddzielna domena (2026-11-04)

**Źródło prawdy:** [PUBLIC_DATASETS.md](PUBLIC_DATASETS.md), [checkpoint dataset sprintu](../checkpoints/2026-11-04-public-datasets-sprint.md). Importer `tools/ml/external_greenhouse_benchmark.py` jest oddzielny od `tools/ml/physics` i od produkcji; tylko Python stdlib, CSV -> pomiar z provenance -> odłożone dni -> baseline persistence -> raport. Nie ma zapisu parametrów solvera, automatyzacji i sieci.

- **7/7 PASS na Mac Local Agent** (stdlib unittest) + **3 rzeczywiste pliki CC BY 4.0, 319 008 wierszy** z walidacją SHA256. Prawdziwy benchmark T/RH na chronologicznie odłożonych dniach 15/60min. Ten nowy wynik nie jest kolejnym testem GPU ani fizycznym dowodem BloomML.
- **Wymuszenia truth boundary:** `provenance=measured_external_processed_not_own_growbox`, `physical_model_calibrated=false`, `training_approval=NOT_APPROVED_EXTERNAL_DOMAIN`. Brak `timezone` w źródle to `source_naive_timezone_unverified`, nigdy domyślne UTC. Przy braku pomiaru brak wartości, nie imputacja.
- **Zmienna aktuatora:** obecność `WINDOW_*`/`HEATERS` nie dowodzi zmierzonego przepływu powietrza ani poboru energii. Filtry braków dla RH trzeba ocenić przed akceptacją celu modelowania; poniżej 80% coverage generowany warning. Zewnętrzny benchmark nie uprawnia do treningu kontrolera.
- **Gates przeciw leakage:** podział po kolejnych **dniach**, testowe punkty startowe i docelowe nie pochodzą z treningu, tylko dokładny horyzont 15 lub 60 min. Dla 1 i 5 min przy danych 15min status `unsupported`; żadnego udawanego oversamplingu. Każdy kolejny model musi używać *identycznego* holdoutu i porównania do persistence.
- **Przywracanie:** `PYTHONPATH=. python -m unittest -v tests.test_external_greenhouse_benchmark`; `PYTHONPATH=. python -m tools.ml.external_greenhouse_benchmark --site alava --file <csv> --expected-sha256 20873873821550b73b9afe95a364c9ac8c572d325143befcd0885650212e3fbd`. Nie commitować danych surowych do Git.

### WUR 2019 — niezależny przykład transferu między domenami

[Raport WUR](WUR_DATASET_EVALUATION.md), [checkpoint](../checkpoints/2026-11-04-wur-2019-dataset-benchmark.md). **16/16 tests PASS na Mac Local Agent**, 6 szklarni i 286 854 rzeczywistych rekordów w publicznej kopii Kaggle (archive SHA-256 przypięty). CSV nie mają zaufanej strefy czasu, `Tot_PAR` wymaga potwierdzenia znaczenia, `sp/vip` nie są dowodem fizycznego wykonania. Wszystkie cele `measured_external_via_unverified_third_party_mirror`; oficjalny 4TU 7z nadal **nieporównany**, `training_approval` nie dopuszcza produkcji. Parser ignoruje brak NaN bez imputacji, flaguje anomalie zamiast je maskować.

**Transfer test:** stały wzór uczony wyłącznie na pierwszych 80% dni innych pięciu szklarni, oceniany na szóstej i ostatnich 20% dni; ten sam zestaw pomiarów co persistence. Dla T60 **6/6 porażek** (MAE +0,8–6,4%), CO₂60 wynik mieszany. **Nie kwalifikować modelu** na podstawie małych dodatnich wyników wybranych foldów. Odrębny od testów fizyki GPU.

## Otwarte ryzyka / jakość naukowa

- **P0:** brak danych terenowych i niepełna entalpia układu otwartego; brak kalibracji modeli wentylatora/filtra, osuszacza, roślin. Wyniki są tylko symulacjami.
- **P1:** porównać 24 skalarne scenariusze na GPU z większą reprezentatywną próbą skrajnych kategorii; test FP32/FP64, testy dłuższych cykli i analiza stresowa rozkładów.
- **P1:** przy refaktoryzacji `growbox_dynamics` uniknąć podwójnej fizyki; nie ruszać bez wersjonowanych golden trajectories i testów parity.
- **P2:** policzyć skuteczność GPU z benchmarkiem CPU na *tym samym* hostcie i horyzoncie, percentyle i perf/VRAM tracking.
- **Następny projekt:** ocenić publiczne szeregi klimatu szklarni, ale **external_domain ≠ growbox real logs**; wymagane licencje, metadane, pomiary i uczciwa ocena generalizacji.

Historia runów i dokładne metryki: [R2](../checkpoints/2026-11-03-r2-torch-parity-sprint.md), [R4](../checkpoints/2026-11-04-r4-dual-t4-stress.md). Stare dokumenty v4, firmware/UI i archiwalne handoffy nie są częścią obecnego audytu.
