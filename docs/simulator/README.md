# BloomML — badania symulatora growboxa

**Start: [AKTUALNY STAN / R0–R8](RESEARCH_STATUS.md)**. Jeden punkt wejścia zamiast powtarzania roadmapy w starych checkpointach. Aktualny tor to **fizyczny solver 0D CPU↔PyTorch GPU**, a nie firmware, UI, stary v4 dataset ani sterowanie produkcyjne.

- [Architektura modułów, testy i observability](QUALITY_AND_OBSERVABILITY.md)
- [Jak wykonać / zweryfikować Kaggle 2×T4](KAGGLE_EXECUTION.md)
- [Założenia fizyczne i ograniczenia](THERMODYNAMIC_CORE.md)
- [Najnowszy checkpoint do wznowienia w nowym czacie](../checkpoints/2026-11-04-bloomml-research-clean-handoff.md)
- [Dowód R4 — 8192 komór, >24 mln podkroków](../checkpoints/2026-11-04-r4-dual-t4-stress.md)

**Aktualny rezultat:** dwa rzeczywiste Tesla T4 przeliczyły identyczne obciążenie R4 przed i po naprawie sztucznego osuszania. Powtórzony run V2 ma bilans wody i próbkę parity CPU↔GPU PASS, ale 25 scenariuszy CO₂>2000 ppm. **Realnych logów nie ma; fizyczna wierność i dane do treningu nadal niezakwalifikowane.**

**Następne działanie:** audyt i dobór publicznych datasetów szklarni/komór uprawowych (GitHub + repozytoria naukowe), z kontrolą licencji, pochodzenia, jednostek, odstępów czasu i zakresu działania aktuatorów. Zewnętrzna szklarnia to osobna domena — nie udajemy, że jest pomiarem naszego growboxa.

Starsze dokumenty `CALIBRATION.md`, `FORMULAS.md`, `PHYSICS_SCOPE.md`, `VALIDATION.md`, `IO_INVENTORY.md`, `SLOT_MAP.md`, `TWIN_VIEW.md`, `SOURCES.md` i `DEPENDENCIES.md` pozostają **referencją/historycznym narzędziem v4**; nie stanowią aktualnego planu tego toru. Produkcyjny climate-v6 opisany jest osobno w `../ARCHITECTURE.md`.

**Granica:** wyłącznie branch `research/simulator-fidelity-kaggle-20261103`, bez zmian `main`, firmware, Growclip i uprawnień Rule/OutputSupervisor.
