# BloomML — kompletny checkpoint: symulator, datasety, ML i gotowość sterowania

**Data checkpointu:** 2026-11-04 · **Zakres:** badania BloomML, bez zmian firmware i bez wydawania komend urządzeniom.

**Przeczytaj jako pierwszy w kolejnej rozmowie.** Następnie [aktualny status](../simulator/RESEARCH_STATUS.md), [wyniki WUR](../simulator/WUR_DATASET_EVALUATION.md), [jakość i observability](../simulator/QUALITY_AND_OBSERVABILITY.md), [ranking datasetów](../simulator/PUBLIC_DATASETS.md) i [dowody obliczeń R4](2026-11-04-r4-dual-t4-stress.md). Dokument jest checkpointem stanu, nie zastępuje `AGENTS.md` ani aktualnego Git HEAD.

## 1. Repozytorium, środowisko, zasady

- GitHub: `MichalMatu/bloomml`. Branch **research/simulator-fidelity-kaggle-20261103**. Odczytany HEAD na początku tego checkpointu: **`28de8253f1d25508faeafdb5b5fdd14eb15fa059`** (po zapisaniu dokumentów HEAD ulegnie zmianie; zawsze ponów odczyt).
- Produkcyjny **`main` = `51c90cd3290efde230f0c929e93aaf620e0de106`** w odczycie przed checkpointem. **Nie zmieniano `main`, Growclip, firmware, Rule ani OutputSupervisor**.
- `agent-control:.agent/status/daemon.json`: `idle`, brak bieżącego zadania, binding `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5` w odczycie wstępnym. Stan nie jest obietnicą trwałej bezczynności.
- Preferowany tryb: **GitHub Connector + ChatGPT Linux Sandbox** dla kodu/testów; **Mac Local Agent** tylko do dozwolonych działań na Macu/Kaggle i potwierdzania śladów. Nie uruchamiać Codex, ChatGPT Work ani GitHub Actions; nie odpytywać Actions. Zadania Local Agent wyłącznie wg kontraktu `AGENTS.md` z `work_branch=main`, `resources=[]`, nigdy nie edytować produkcji, jeśli praca dotyczy research branch.
- Repo zawiera odrębny produkcyjny system sterowania na ESP32-S3. Historyczne dokumenty climate-v4/v6, UI i firmware **nie są** aktywną roadmapą symulatora 0D.

## 2. Co jest rzeczywiście ukończone

| Obszar | Zweryfikowany wynik | Granica dowodu |
| --- | --- | --- |
| R0/R4 fizyka CPU | Referencyjny symulator 0D: temperatura powietrza/liścia/ścian/podłoża, RH/VPD, para/mgła, kondensacja, CO₂, światło, wentylacja, bilanse wody; poprawione sprzężone ciepło utajone, szacowana krzywa wentylatora i osuszacz kondensacyjny | Stabilność i inwarianty numeryczne, **bez kalibracji** rzeczywistej komory; uproszczona entalpia otwartego układu |
| R1/R2 GPU | PyTorch CUDA FP64; faktyczna fizyka uruchomiona na **2 oddzielnych Tesla T4**, a nie tylko smoke test CUDA | Niewielki pilot CPU↔GPU, nie pełne empiryczne dopasowanie; brak benchmarku przyspieszenia vs ten sam host CPU |
| R4 ciężki eksperyment | **8192** komory × **180** kroków, łącznie **24 424 020 wewnętrznych podkroków** fizyki; przed/po korekcie osuszacza, oba GPU `COMPLETE` | V2: RH min **6,65%** vs V1 **0,0133%**, bezwzględny błąd bilansu wody max ~**3,8e-14 kg**, CPU↔GPU pilot parity ~**5,7e-14**; 25 scenariuszy CO₂ > 2000 ppm. Tylko `NUMERICAL_EVIDENCE_PASS` |
| Jakość kodu | Historycznie **63/63 testów symulatora/audytu PASS w ChatGPT Linux Sandbox**; AST import-boundaries dla 6 modułów i niezależny walidator manifestów | Wynik dotyczy oznaczonego snapshotu; nie raportować jako bieżący pełny retest każdego późniejszego commitu |
| NUTRICONTROL (Hiszpania) | Trzy autentyczne CSV (Álava, Cartagena, Navarra), **319 008** rekordów; CC BY 4.0; SHA256, czas i braki sprawdzone; **7/7 testów importu PASS na Macu** | T/RH do zewnętrznego benchmarku, różna kompletność; brak CO₂/bezpośredniego PPFD; nie jest to własny growbox |
| Wageningen AGC 2019 | Publiczny mirror Kaggle (CC0) z **6 × 47 809 = 286 854** próbkami klimatu i **47 809** pogody, T/RH/CO₂/PAR co ~5 min, ~99,85% kompletności; **16/16 testów WUR PASS na Macu** | Oryginalny 7z 4TU **niepobrany** (timeout API), zgodność bajtowa z mirrorem **niepotwierdzona**; czas Excela bez potwierdzonej strefy; `Tot_PAR` wymaga kontroli semantyki |
| WUR: pierwszy trening/transfer | Jeden parametr trendu wyuczony na pierwszych 80% dni **pięciu innych szklarni** i oceniony na ostatnich 20% dni niewidzianej szóstej, porównanie na identycznych parach z persistence, horyzonty 5/15/60 min | **Nie pokonał persistence dla temperatury 60 min w żadnej z 6 szklarni**; nie traktować tego jako udanego modelu sterowania |
| Obserwowalność | SHA i hashe danych, wersja źródła/licencja, brakujące i niepoprawne próbki, znaczenie kanałów, train/test split, per-target/horizon MAE/RMSE/bias; osobne alarmy dla podejrzanych wartości | WUR Reference: 2 niskie Tair (min −1°C), 1 niska RH (7,7%), 2 niepoprawne CO₂ i 3 RH. Flagowanie ≠ autokorekta; brak rzeczywistego feedbacku urządzeń |

**Dowody z Local Agenta (status `done`, exit 0 w odczycie):**
- `agent-control:.agent/results/la-bloomml-r4-corrected-kaggle-verify-20261104-v2.json` — dwa GPU, manifest, bilans, parity.
- `agent-control:.agent/results/la-bloomml-public-datasets-baseline-20261104-v1.json` — trzy CSV, 7 testów i persistence.
- `agent-control:.agent/results/la-bloomml-wur-unseen-household-forecast-20261104-v2.json` — test transferu i wynik ujemny dla T60.
- `agent-control:.agent/results/la-bloomml-wur-qc-extremes-20261104-v1.json` — **16 testów** i anomalie WUR.
- [Raport szczegółowy WUR i tabelki sześciu szklarni](../simulator/WUR_DATASET_EVALUATION.md); [raport R4](2026-11-04-r4-dual-t4-stress.md). **Nie sumować mechanicznie 63 + 7 + 16 jako jednego retestu na bieżącym HEAD**.

## 3. Dokładna interpretacja „1% odchyłu” i wyników

**Bardzo ważne:** w eksperymencie WUR liczby **0,8–6,4% pogorszenia** oznaczają wzrost **MAE modelu względem MAE persistence**, a NIE błąd prognozy temperatury = 0,8–6,4% temperatury i NIE błąd sterowania. Dla przykładu AICU prognoza na 60 min: persistence **0,969°C MAE**, model trendu **0,988°C MAE** (~2% względnie gorzej); CO₂60 AICU **51,88 ppm → 50,78 ppm**, niewielka poprawa. W innych szklarniach zmiany CO₂ są mieszane; T60 gorzej 6/6.

**Nie określać akceptacji jednym procentem.** Procent temperatury w skali °C bywa mylący (zero skali jest umowne), a 1% błędu predykcji nie oznacza 1% błędu procesu sterowania. Raportuj:
- temperaturę: **MAE/RMSE/bias w °C**, osobno P95/P99 i maksymalne przekroczenia limitu;
- RH: w **punktach procentowych**, VPD w **kPa**;
- CO₂: w **ppm**, osobno liczbę i czas niebezpiecznych/niepożądanych epizodów;
- oświetlenie: w właściwych jednostkach PAR/PPFD, DLI i rzeczywistym sygnale urządzeń;
- decyzje sterowania: **czas poza zadanymi granicami**, opóźnienie reakcji, oscylacje/częste przełączenia, energia/woda, stany awarii.
- Dopuszczalne tolerancje **nie zostały zatwierdzone** dla własnego growboxa; muszą wynikać z celu uprawy, dokładności/calibration czujników, szybkości zmian i właściwości aktuatorów. Ilustracyjne progi (np. 0,3–0,5°C, 2–3 pp RH lub 50–100 ppm CO₂) to **hipotezy projektowe**, nie wyniki badań i nie policyjny safety gate.

## 4. Czy ML może sterować wentylacją, lampami, nawilżaniem itd.?

**Technicznie tak, ale nie przy obecnym poziomie dowodów.** Obecny BloomML jest `RESEARCH/SHADOW_ONLY`. Nie wolno przekazywać rekomendacji bezpośrednio do przekaźników. Docelowy bezpieczny kierunek:

```text
Sensory i rzeczywiście wykonane działania + jakość/starość odczytów
         ↓
 Estymacja stanu → model prognostyczny ML / solver (wraz z niepewnością)
         ↓
 Propozycja nastaw (bez prawa wywołania fizycznych wyjść)
         ↓
 Deterministyczne Rule / limity / priorytety / timeout / interlocki
         ↓
 OutputSupervisor (jedyny właściciel fizycznych wyjść)
         ↓
 Weryfikacja wykonania + telemetria → obserwowalność / analiza
```

- **Aktualny status:** ML tylko obserwator/prognoza; nie ma nowego kodu produkcyjnego, endpointu sterującego ani testu hardware w tym checkpointcie.
- **Wariant przyszły:** kolejne etapy to `OFFLINE` → `SHADOW` (loguje propozycje bez wykonania) → `LIMITED_SUPERVISED` (po odrębnym zatwierdzeniu, z konserwatywnym deterministycznym arbitrażem) → ewentualnie `QUALIFIED`. To **proponowane etapy**, nie opis zaimplementowanych przełączników.
- **Obowiązkowy fail-safe:** brak/stare/niepewne sensory, inferencja timeout, out-of-domain inputs, brak potwierdzenia wyjścia, przekroczenie limitów lub awaria sieci → deterministyczny bezpieczny stan/Rule, nie decyzja ML. Każda automatyka dotycząca CO₂, temperatury i innych aktuatorów wymaga własnej analizy ryzyka.
- **Jeden właściciel wykonywania:** `OutputSupervisor`; `Rule` jest autorytetem bezpieczeństwa/polityki; warstwa ML nie może przejąć RF433/transportu ani obejść blokad.
- **Obserwowalność decyzji:** wejście/feature freshness i provenance, model ID+SHA, prognoza i niepewność, propozycja, wersja policy, akceptacja/odrzucenie i powód, finalne wyjście, obserwowane potwierdzenie, korekta odchyłki, safety override. Stan żądany ≠ wykonany.

## 5. Warunki kwalifikacji przyszłej automatyki ML (OPEN/BLOCKED)

1. **Źródłowe dane:** prawdziwe własne logi growboxa, synchronizacja czasu, czujniki T/RH/CO₂/światła oraz *wykonane*, nie tylko żądane stany lamp/wentylacji/osuszacza/grzania; źródło i kalibracja czujników. **Brak własnych logów obecnie = BLOCKED**.
2. **Wierny model urządzeń:** zweryfikowane parametry przepływu/ΔP, mocy i bezwładności, osuszania i wymiany ciepła; pełniejszy bilans entalpii oraz zakresy obowiązywania. **OPEN**.
3. **Zewnętrzny baseline:** benchmark na tych samych testowych dniach i komorach, bez leakage, z porównaniem persistence, prostymi modelami i fizyką; wykazać korzyści i stabilność między obiektami. **WUR T60 obecnie FAIL vs persistence**.
4. **Niezależny holdout:** inne dni/sytuacje i docelowy growbox, rozkłady błędów, P95/P99, niepewność/odchylenie domeny, brak fałszywej zgodności z symulatorem. **BLOCKED dla własnej domeny**.
5. **Sterowanie offline:** closed-loop w wielu seedach, skrajne scenariusze, brakujące sensory, awarie, niezgodności stanu aktuatora; testy wnioskowania i shadow nie wpływają na przekaźniki. **NOT RUN**.
6. **Shadow hardware:** trwały zapis decyzji ML, reakcji Rule/OutputSupervisor i prawdziwego potwierdzenia wyjść przez odpowiedni czas; stop/rollback na odchyłkach. **NOT RUN**.
7. **Dopiero formalne pozwolenie na LIMITED_SUPERVISED:** niezależnie zaakceptowane limity, interlocki, manual override, watchdog, fail-safe, limity przełączania/histereza, eskalacja i szybkie wyłączenie funkcji ML. **NOT APPROVED**.

**Quality gate na dzień checkpointu:** `NUMERICAL_SIMULATOR_PASS`; `EXTERNAL_DATASET_QA_PASS_WITH_WARNINGS`; `CROSS_GREENHOUSE_ML_T60_BELOW_BASELINE`; `OWN_GROWBOX_EMPIRICAL_VALIDATION_BLOCKED`; `ML_ACTUATION_AUTHORITY_DENIED`. Nie używać jednego zbiorczego zielonego `PASS` dla całego celu.

## 6. Modułowość / observability / dług techniczny

- `tools/ml/physics/growbox_dynamics.py` (~600 linii) wymaga planowanej refaktoryzacji **dopiero** z zachowaniem charakterystyki i parity CPU↔Torch; powielone równania skalar/tensor wymagają testów regresji.
- `tools/ml/research_evidence.py`: *read-only* kontrola schematu, SHA, dwóch odrębnych CUDA, fizycznych bounds/bilansu, małego pilota parity, bez przypisywania walidacji empirycznej.
- `tools/ml/external_greenhouse_benchmark.py`: NUTRICONTROL, SHA/15min/holdout i status `measured_external_processed_not_own_growbox`.
- `tools/ml/wur_external_benchmark.py`: bez ekstrakcji dane ZIP WUR mirror, serial 5min, 6 komór, braki/outliery, 5/15/60min baseline i `NOT_VERIFIED` byte-equivalence.
- `tools/ml/wur_forecast_probe.py`: **oddzielny** jednoparametrowy model trendu do testu transferu; to nie solver fizyczny, sterownik ani końcowy model ML.
- Ważne: wyniki z Kaggle, pliki pobrane z 4TU/Kaggle, SHA, pomiary, train/test split, warnings i faktyczne działania zachować jako rozłączne typy dowodów. Duża liczba GPU-operations ≠ jakość prognozy ≠ gotowość do sterowania.
- **Obecnie brak** rzeczywistej historii decyzji ML → Rule → OutputSupervisor, bo ML nie wykonywał kontroli w tym badawczym przebiegu. Nie twierdzić, że observability hardware jest gotowe.

## 7. Następny jeden sprint (bez powtarzania zamkniętych badań)

**Priorytet P1: WUR provenance + model z wymuszeniami, nadal offline.**
1. Odzyskać oryginalny 4TU WUR 7z i dokumentację; porównać **wewnętrzne pliki** z mirrorem, jednostki Tot_PAR, semantykę `co2_dos`, `*_vip`, `VentLee`, strefę czasu. Jeżeli API nadal nie odpowiada, utrzymać `MIRROR_NOT_VERIFIED`, nie blokować samej analizy eksploracyjnej, ale nie promować do calibrated ground truth.
2. Dopiero po wyjaśnieniu kanałów dodać cechy dostępne **najpóźniej w chwili prognozy**: T/RH/CO₂ historyczne, Tout/Rhout, PAR, lampy, sygnały sterowania z poprawnym `requested/measured/derived`; brak przyszłego weather/setpoint/realized actuation.
3. Testować **5/15/60 min** na niewidzianej komorze i późniejszych dniach, porównać z **dokładnie tą samą** persistence; raportować osobno MAE, RMSE, bias, per greenhouse, missingness, failure cases i powody braku poprawy.
4. Zbiór koreański `pf24hxwygf/1`: niezależny import `measured` vs `simulated`, ale nie mieszać go z WUR i nie udawać fizycznej kalibracji growboxa.
5. Gdy będą własne logi, wykonać R6: kalibrację, holdout innego dnia/warunków, error/safety envelope; potem dopiero shadow w realnym systemie.

**Kryterium zakończenia następnego sprintu:** pinned source + schema/clock/provenance + testy negatywne + uczciwy baseline i raport; jeśli nie ma wiarygodnej poprawy, zapisać **FAIL/BLOCKED** i wrócić do cech/fizyki zamiast deklarować sukces.

## 8. Handoff / odtworzenie

Przed dowolną zmianą odczytać w kolejności: `AGENTS.md`, `docs/README.md`, `docs/CURRENT_STATUS.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md`, `docs/PROJECT_LAYOUT.md`, `docs/simulator/RESEARCH_STATUS.md`, **ten checkpoint**, aktualne HEAD `main` i research branch oraz `agent-control:.agent/status/daemon.json`. Stare SHA służą tylko identyfikacji evidence.

```bash
# historyczny test WUR, nie automatyczny dowód bieżącego HEAD
python3 -m unittest -q tests.test_wur_external_benchmark tests.test_wur_forecast_probe

# historyczny pełny zestaw fizyki (Torch na CPU w Linux Sandbox)
PYTHONPATH=. python -m pytest -q \
  tests/test_growbox_thermodynamics.py \
  tests/test_growbox_measurements.py \
  tests/test_growbox_edge_checkpoint.py \
  tests/test_growbox_randomized_edge_cases.py \
  tests/test_growbox_sweep.py \
  tests/test_growbox_tensor.py \
  tests/test_kaggle_gpu_r2.py \
  tests/test_growbox_r4.py \
  tests/test_growbox_r4_convergence.py \
  tests/test_kaggle_r4_stress.py \
  tests/test_research_evidence.py
```

Dla rzeczywistych danych WUR: sprawdzić `MIRROR_SHA256`, `source_clock`, `provenance`, status oryginału, nie commitować raw ZIP i nie wkładać prywatnych logów do publicznego repo. Dokumentacja źródła: [WUR](../simulator/WUR_DATASET_EVALUATION.md); [R4](2026-11-04-r4-dual-t4-stress.md); [pełny checkpoint historyczny przed WUR](2026-11-04-bloomml-research-clean-handoff.md).

**Syntetyczne wyniki i modele mogą być używane do badania jakości kodu oraz eksploracji, ale obecny BloomML NIE MA zgody na sterowanie fizycznym osprzętem.**

**Stan checkpointu:** `FULL_RESEARCH_AND_CONTROL_READINESS_HANDOFF`; `REPO_AND_AGENT_PREFLIGHT_VERIFIED`; `ALL_NEW_CHANGES_DOCUMENTATION_ONLY`; `ML_CONTROL_NOT_QUALIFIED`; `NEXT_WUR_SOURCE_AND_INPUT_AWARE_EXPERIMENT`.
