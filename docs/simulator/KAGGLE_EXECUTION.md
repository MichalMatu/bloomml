# Kaggle → BloomML: tryb obliczeniowy T4 ×2 (offline research)

**Źródło kodu:** MichalMatu/bloomml, branch research/simulator-fidelity-kaggle-20261103.
**Handoff i plan:** ../checkpoints/2026-11-03-kaggle-bloomml-compute-handoff.md.
**Status:** notebook i kontrakt artefaktów są w repo, fizyczne GPU physics i rzeczywiste uruchomienie Kaggle jeszcze niezweryfikowane.

## Uruchomienie

1. W Kaggle stwórz **prywatny** notebook. W Settings włącz Internet (na czas git clone) i wybierz **GPU T4 x2**. Zakończ poprzednie sesje Ollama, jeśli zajmują limity GPU.
2. Zaimportuj plik notebooks/BloomML_Kaggle_T4x2.ipynb z GitHub. Notebook domyślnie klonuje wyłącznie branch badawczy. Opcjonalny PINNED_SHA służy do odtworzenia wcześniej sprawdzonego commita. Nie zgaduj SHA.
3. Uruchom komórki po kolei. Pierwsza wyświetla dokładny SHA źródeł; druga uruchamia dwa rzeczywiste mini-obliczenia CUDA i CPU golden.
4. Zapisz /kaggle/working/bloomml-run/kaggle_manifest.json i cpu_golden.ndjson jako **prywatne** outputy. Zachowaj source_commit i digest z manifestu.
5. Po powrocie do projektu porównaj manifest z odczytanym commitem GitHub. Nie przypisuj wyników innemu SHA. Nie kopiuj surowych logów growboxa na publiczne konto Kaggle.

## Własności i ograniczenia

Notebook wykonuje polecenie analogiczne do:

    python -m tools.ml.kaggle_physics_runner --source /kaggle/working/bloomml-source --output /kaggle/working/bloomml-run --scenarios 4 --steps 120 --seed 1729 --require-gpus 2

Runner:
- sprawdza dostępność CUDA i co najmniej dwóch GPU za pomocą PyTorch;
- na każdym GPU wykonuje kontrolny iloczyn macierzy;
- na CPU generuje wersjonowany złoty przebieg termodynamiczny i sprawdza bilans wody;
- zapisuje źródłowy SHA i manifest bez sekretów.

**Uwaga:** na tym etapie realna fizyka wykonywana jest na CPU. Nawet jeśli test CUDA przechodzi, nie można raportować “GPU physical simulation completed”. Kolejny krok to backend Torch/JAX batched i parity CPU↔GPU.

Błędy:
- brak Internetu → niepowodzenie git clone (włącz Internet albo użyj kontrolowanego snapshotu);
- brak dwóch kart → jawny wyjątek: NIE przyjmuj działania tylko na CPU jako sukcesu etapu R1;
- zbyt duża sesja Kaggle → ogranicz liczbę scenariuszy/kroków, zapisz manifest i artefakty, nie polegaj na ciągłej pracy przez tydzień;
- CUDA smoke działa, ale CPU golden zawodzi → błąd modelu/testu, NIE deklaruj powodzenia;
- zmiana commita podczas pracy → nie dotyczy już sklonowanego checkoutu, bo runner zapisuje exact SHA; następny run ponownie rozstrzyga branch.

## Następny etap: rzeczywiste dwa GPU

Należy dodać wyraźną implementację solvera tensorowego w tools/ml/physics (oddzielny moduł) i testy:
- CPU↔GPU jednakowy stan, wektor parametrów, sterowania, horyzont czasu i metoda całkowania;
- porównanie energii, masy pary/CO₂, temperatur, kondensatu i VPD; nie tylko końcowego RH;
- zbieżność dt i tolerancje FP32 vs FP64, brak NaN/Inf, ograniczenia fizyczne i brak cichego fallbacku;
- podział niezależnych scenariuszy na cuda:0 i cuda:1; procesy/strumienie z jawną własnością VRAM;
- wall-clock, zużycie pamięci, throughput i pełna provenance. Duże wyniki wyłącznie w artefaktach Kaggle/prywatnym storage, nie commitować danych masowych.

## Granice projektu

To NIE jest API lokalnego agenta ani tunel do ESP32. To notebook badawczy pobierający kod z repozytorium i oddający pliki wynikowe. Nie wymaga tokena GitHub do publicznego repo, a do prywatnego Kaggle notebooka zalogowany użytkownik korzysta ze swojego konta. Nie wpisuj w notebooku kluczy ani tokenów. Brak harmonogramu i automatycznego wysyłania wyników z Kaggle do GitHub.

Produkcyjne Rule i OutputSupervisor zostają autorytetem. Kod firmware i Growclip bez zmian.
