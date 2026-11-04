# BloomML / symulator fizyczny — START HERE

**Stan: 2026-11-04.** Repo `MichalMatu/bloomml`, branch `research/simulator-fidelity-kaggle-20261103`. Jest to badawczy model klimatu growboxa (0D, jedna dobrze wymieszana strefa), **nie cyfrowy bliźniak skalibrowany pomiarami**. **Nie mamy jeszcze realnych logów czujników.**

## Po co to robimy

Chcemy przewidywać temperaturę powietrza, liści, ścian i podłoża, wilgotność, parę/mgłę/kondensat, CO₂, VPD, bilans wody oraz wpływ lamp, ogrzewania, chłodzenia, cyrkulacji i wyciągu. Celem jest model, który można *najpierw* zweryfikować na fizyce i pomiarach, a dopiero później wykorzystać do przygotowania danych ML dla ESP32-S3 w trybie shadow.

## Co już działa

- **CPU reference:** deterministyczny solver termodynamiczny, bilanse i testy skrajnych przypadków (R0).
- **Torch batched:** obliczenia tych samych równań na CPU i GPU; niezależne partie na `cuda:0` i `cuda:1` (R2).
- **Kaggle:** zweryfikowano dwa rzeczywiste Tesla T4, a w dwóch dużych przebiegach R4 wykonano po **8192 scenariusze / 24 424 020 podkroków**. V1 ujawnił fałszywe osuszanie; V2 po poprawce wykazał minimalne RH **6,65%** (zamiast 0,0133%), maksymalny błąd bilansu wody ok. **3,8e-14 kg**. Oba GPU `COMPLETE`.
- **Weryfikacja lokalna:** 51/51 testów istniejącego rdzenia; nowy zestaw walidatora evidence i zależności doprowadził testy badawcze do **63/63 PASS w sandboxie**. Weryfikacja GPU dotyczy udokumentowanego SHA V2: `05953e0139cfc31c7ba1fde6ba181d6a90bb9fb3`, a nie automatycznie wszystkich późniejszych commitów dokumentacji.
- **Obserwowalność:** manifest źródłowego SHA, seed, czasów i pamięci GPU, liczby scenariuszy, dryfu CPU↔CUDA, bilansu wody, T/RH/CO₂ oraz liczników ekstremów. Nowy audyt `tools/ml/research_evidence.py` raportuje `NUMERICAL_EVIDENCE_PASS` bez fałszywej deklaracji walidacji empirycznej.

## Co pozostaje otwarte

- Nie ma kalibracji ani niezależnych pomiarów z growboxa; **nie wolno twierdzić, że przewidywania są rzeczywiste lub nadają się już do treningu produkcyjnego**.
- Pełny bilans suchego powietrza i entalpii układu otwartego, zmierzone krzywe wentylatora/filtra, charakterystyka osuszacza, fizjologia roślin i przestrzenność — nadal uproszczone.
- 25 z 8192 scenariuszy V2 osiągnęło CO₂ powyżej 2000 ppm. To obserwacja do dalszej kontroli, nie sukces agronomiczny.
- Nie wykonano uczciwego benchmarku *ten sam host CPU vs GPU* ani szerokiego testu FP32; **nie raportujemy przyspieszenia ani zalecenia FP32**.
- `growbox_dynamics.py` (~600 linii) wymaga planowej dekompozycji z testami charakterystyki; CPU i tensor mają dwa zapisy części równań i ryzyko driftu.

## Kolejność prac R0–R8

| Etap | Stan / warunek |
| --- | --- |
| R0 | **PASS numerycznie:** rdzeń 0D i inwarianty; bez kalibracji |
| R1 | **DONE:** dostęp do prawdziwych T4, CPU oracle, Kaggle |
| R2 | **DONE dla FP64 pilota i stressu:** rzeczywisty solver 2× GPU, ograniczona próbka parity; pełne FP32 i benchmark otwarte |
| R3 | **CZĘŚCIOWO:** importer SD istnieje, logów realnych obecnie brak |
| R4 | **CZĘŚCIOWO PASS:** kondensacja/energia, charakterystyka wyciągu opcjonalna, osuszacz skorygowany; pełna entalpia nie |
| R5 | **WARUNKOWO:** multizone dopiero przy dowodach gradientów |
| R6 | **BLOKADA:** kalibracja i test na osobnych dniach wymagają realnych pomiarów |
| R7 | **BADANIA SYNTETYCZNE wykonane**, ale trening na realistycznych danych wstrzymany do R6 |
| R8 | **NIE ROZPOCZĘTO:** TinyML shadow-only po kwalifikacji |

## Zewnętrzne datasety — sprint discovery + pierwszy baseline (2026-11-04)

**Discovery wykonany:** [ranking i raport wyników na realnych plikach](PUBLIC_DATASETS.md). Pierwszy source: **NUTRICONTROL CC BY 4.0**, przypięte 3 CSV z 318 008? faktyczna liczba sprawdzonych wierszy **319 008**: Álava 80 548, Cartagena 69 792, Navarra 168 668. Kontrola sha256, dat, braków i rzeczywisty baseline persistence na końcowych 20% dni: Álava MAE temperatury 15 min **0,384°C**, 60 min **1,169°C**. Cartagena RH tylko 39,6% kompletna; brak CO₂/PPFD we wszystkich tych trzech plikach.

- **Kod i testy:** `tools/ml/external_greenhouse_benchmark.py`, `tests/test_external_greenhouse_benchmark.py` — **7/7 PASS na Mac Local Agent**; tylko Python stdlib, bez interpolowania, bez przecieku chronologicznego, bez przyjmowania niezweryfikowanej strefy czasowej.
- **Następny krok:** przetestować oryginalne archiwum Wageningen AGC 2019 **CC0** (CO₂/PAR, 6 komór i urządzenia); później Korea 2026 do walidacji T/RH. Metadane kandydatów sprawdzone, ale surowych plików Wageningen i Korei nie importowano.
- **Granica:** tylko zewnętrzna domena pomiarów, benchmark = persistence, nie wynik symulatora, **brak własnych logów i kalibracji**, zero treningu produkcyjnego.

## Gdzie wejść

- [Audyt zewnętrznych danych i baseline](PUBLIC_DATASETS.md)
- [Mapa kodu, granice i jakość](QUALITY_AND_OBSERVABILITY.md)
- [Kaggle / Local Agent](KAGGLE_EXECUTION.md)
- [Fizyka i założenia](THERMODYNAMIC_CORE.md)
- [Kompletny checkpoint dla następnego czatu](../checkpoints/2026-11-04-bloomml-research-clean-handoff.md)
- [Dowód ciężkiego runu R4](../checkpoints/2026-11-04-r4-dual-t4-stress.md)

**Granica bezpieczeństwa:** praca tylko na branchu badawczym, bez zmian `main`, firmware, Growclip. Produkcyjne Rule/OutputSupervisor pozostają autorytetem. Stary v4/v6 toolchain i UI nie są aktywnym zakresem tego projektu.
