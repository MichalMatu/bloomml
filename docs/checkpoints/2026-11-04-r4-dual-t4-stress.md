# BloomML R4 — duże obliczenia Kaggle, korekta fizyki i ocena wiarygodności

Data: 2026-11-04. Repo `MichalMatu/bloomml`; branch `research/simulator-fidelity-kaggle-20261103`.
Zakres: **wyłącznie offline research**. Nie zmieniać `main`, firmware, Growclip, Rule ani OutputSupervisor.

## Zmiany symulatora

- `tools/ml/physics/growbox_phase.py`: rozwiązanie sprzężonego bilansu pary wodnej, mgły i ciepła utajonego (8 iteracji Newtona z granicami temperatury); parowanie mgły ma stałą czasową 60 s, a nie zależy od liczby podkroków. To nadal **niekalibrowana** kinetyka 0D, nie model aerozoli.
- `growbox_dynamics.py`, `growbox_tensor.py`: zgodne formuły skalara i PyTorch dla fazy wodnej; przepływ wentylatora na opcjonalnej krzywej Q(ΔP), wyłącznie przy podanym `exhaust_shutoff_pressure_pa`; bez tej informacji tylko estymacja z tabliczki znamionowej. Rzeczywiście zmierzony przepływ ma pierwszeństwo.
- W wentylacji osobno uwzględniono sensible ciepło właściwe suchego powietrza i pary; **pełny bilans entalpii układu otwartego nadal nie jest rozwiązany**.
- W pierwszym obciążeniu wykryto sztuczne osuszanie aż do 0.0133% RH: dotychczasowy model usuwał wilgoć ze stałą szybkością przy dowolnie małym ciśnieniu pary. Dodano `dehumidifier_coil_depression_c` (prior 12 C) i ograniczenie efektywności osuszacza kondensacyjnego różnicą wilgotności względem nasycenia na chłodnicy. Parametr wymaga pomiaru; nie zakładamy, że symulator odtwarza konkretną jednostkę.
- `kaggle_r4_stress_runner.py`: pięć kategorii warunków, dwie CUDA, pilot scalar CPU↔CUDA, bilans wody, kontrola temperatury/RH/CO2, licznik ekstremalnie suchego powietrza (RH<5%) i wysokiego CO2 (>2000 ppm). Przypadki nie są danymi pomiarowymi.

## Weryfikacja lokalna

- **51/51 PASS** Python + PyTorch CPU, w tym testy pomiarów, fazy, charakterystyki wentylatora, ograniczenia osuszania, błędnych warunków i zgodności tensorowej.
- **40 zróżnicowanych przypadków**: test zbieżności kroku 1 s kontra 10 s (5 minut) PASS z limitami |ΔT|<0.005 C, |ΔRH|<0.05 punktu procentowego, |ΔCO2|<0.5 ppm.
- **1000 syntetycznych przypadków fazowych**: maks. błąd zamkniętego bilansu ciepła ~8.73e-11 J, wody ~2.78e-17 kg.
- Powyższe nie są dowodami dokładności predykcji względem realnego growboxa.

## Kaggle — rzeczywiste dwa GPU i ciężki workload

Notebook prywatny: https://www.kaggle.com/code/meeehooow/bloomml-r4-physics-stress

Parametry obu wersji: 2× Tesla T4; 8192 heterogenicznych komór; 180 ticków na komorę × 10 s = 30 minut czasu modelowego; **1 474 560 scenario ticks, 24 424 020 wewnętrznych podkroków Eulera**. Każda T4 wykonuje własne scenariusze, niezależna pamięć GPU. GPU użyto do **pełnego wektorowego solvera**, nie tylko testu CUDA.

### V1 — wykrycie anomalii

- Source SHA: `c9e5d2e37e02aa345d25606120416b357175d296`.
- GPU0: 12 212 460 podkroków, 167.61 s; GPU1: 12 211 560 podkroków, 176.62 s.
- Maksymalny błąd bilansu wody 5.51e-14 kg; CPU↔CUDA parity PASS.
- Min RH **0.0133%**, max CO2 4450 ppm. To nie jest potwierdzenie realistycznego osuszania; anomalia spowodowała korektę modelu.
- Saved Kaggle run v1 `KernelWorkerStatus.COMPLETE`; manifest odzyskano przez API po poprawieniu wzorca pobierania.
- Evidence: `agent-control:.agent/results/la-bloomml-r4-kaggle-diagnose-manifest-20261104-v1.json`.

### V2 — powtórny identyczny stress po korekcie osuszacza

- Source SHA: **`05953e0139cfc31c7ba1fde6ba181d6a90bb9fb3`**.
- GPU0: Tesla T4, 12 212 460 podkroków, **168.69 s**, peak torch allocated 6 807 552 B, max błąd scalar CPU↔CUDA **5.68e-14**.
- GPU1: Tesla T4, 12 211 560 podkroków, **177.88 s**, peak torch allocated 6 807 552 B, max błąd scalar CPU↔CUDA **1.07e-14**.
- Obie karty: **0 fail physical bounds**, max |water residual| **3.78e-14 kg**.
- Min/max T powietrza: **9.65–34.69 C**. Min RH **6.65%** (po poprzednich 0.0133%), max RH 100%. **0 przypadków RH<5%**.
- Max CO2 **4450.08 ppm**; **25/8192 przypadków przekroczyło 2000 ppm** (13 na GPU0, 12 na GPU1). Może to wynikać z syntetycznego dozowania CO2 i małej wentylacji; stężenia nie zostały sztucznie obcięte, ale wymagają analizy agronomicznej i bezpieczeństwa.
- W sumie po korekcie RH średnie ~49.5%. Rzeczywiste osuszacze, drożność przepływu i mikroklimat roślin pozostają niezmierzone.
- Kaggle version #2 `COMPLETE`, manifest `r4_stress_manifest.json` pobrany i zweryfikowany przez Mac Local Agent.
- Evidence: `agent-control:.agent/results/la-bloomml-r4-corrected-kaggle-verify-20261104-v2.json` (status done, exit code 0, `R4_V2_DUAL_T4|PASS_NUMERICAL_ONLY`).

## Ocena naukowa — co ma sens, a co jeszcze nie

**PASS (numerycznie):** równowaga parowania i kondensacji z ciepłem utajonym, bilans wody, granica RH≤100%, brak ujemnych zapasów kondensatu, zbieżność kroku, zgodność scalar CPU↔CUDA przy wybranych próbkach. Obydwa GPU rzeczywiście wykonywały termodynamikę.

**PHYSICALLY PLAUSIBLE, NOT QUALIFIED:** T~9–35 C, RH~7–100%, wysokie stężenia CO2 w zamkniętych małych komorach z dozownikiem są możliwymi scenariuszami wymuszenia. Jednak zakresy te nie są dowodem, że konkretny growbox zachowuje się tak samo; część przypadków jest niepożądana dla upraw.

**NOT VALIDATED:** strumienie powietrza i opory filtra/fan-curve nie zostały zmierzone, otwarty bilans suchego powietrza i całej entalpii pozostaje uproszczony, brak rozkładów przestrzennych, widma/promieniowania, mechanistycznej fizjologii roślin i dopasowania parametrów. `dehumidifier_coil_depression_c=12` to prior, a nie pomiar. Ani miliony syntetycznych przypadków, ani CPU parity nie zastąpią kalibracji na logach SD.

**Nie deklarować przyspieszenia GPU vs CPU:** nie przeprowadzono uczciwego porównania tego samego pełnego workloadu na tym samym hostcie CPU. Peak memory to alokacje PyTorch, nie pełne zajęcie VRAM. Oba GPU liczyły rzeczywistą fizykę; w przyszłości profilować efektywność i wybrać FP32 po osobnym parity.

## Następny sprint

1. Zabezpieczyć dane SD (R3) z sygnaturami czasu i realnym potwierdzeniem wykonania urządzeń; czujniki T/RH/CO2, lamp W/PPFD, temperatury liści/ścian, przepływ/ΔP, woda/odciek.
2. R4 dalej: pełny bilans masy suchego powietrza i entalpii oraz charakterystyki osuszacza/wyciągu i filtra **na podstawie pomiarów**.
3. R6: kalibracja parametrów na dniach treningowych i test na **innych dniach**, porównanie z prostą prognozą persistence, MAE/RMSE/bias, forecast 1/5/15/60 min, nie tylko dopasowanie trajektorii syntetycznych.
4. R7: duże ensemble dopiero po pozytywnym R6; R8 TinyML ESP32 shadow-only. Produkcja Rule/OutputSupervisor bez zmian.

**Status:** R2 dual-GPU heavy numerical stress DONE; R4 conservation/phase/dehumidifier slice DONE numerically, complete physical fidelity OPEN; R3 partial, R6 OPEN, R7 calibration-blocked.
