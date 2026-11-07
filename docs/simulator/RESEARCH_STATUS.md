# BloomML — aktualny status badań (START HERE)

**Badania 2026-11-04, konsolidacja 2026-11-07** · kod i historia eksperymentów na `main`. **Jeden aktualny checkpoint do wznowienia MPC:** [MPC complete resume](../checkpoints/2026-11-04-bloomml-mpc-resume-complete.md). **Plan:** [MPC_ROADMAP.md](MPC_ROADMAP.md). Nie mieszać starszego v4 research z climate-v6 produkcji.

| Obszar | Zweryfikowane | Obecny gate |
| --- | --- | --- |
| R4 / fizyka 0D | CPU model, bilans wody, Torch FP64 parity pilot; R4 V2 8192 komór na **2×T4**, 24 424 020 podkroków, RH min 6.65%, 25 przypadków CO₂>2000 ppm | **Numerycznie PASS w opisanym zakresie; fizyczna wierność UNKNOWN** |
| Zewnętrzne dane | NUTRICONTROL 319 008 rekordów / 3 CSV, WUR mirror 286 854 pomiarów / 6 szklarni; na WUR trend T60 gorzej niż persistence **6/6** | Tylko zewnętrzny benchmark; nie kalibruje własnego growboxa |
| MPC A–C | Modułowe urządzenia, search, laboratorium; 60-min epizod MPC T/RH/CO₂ MAE 1.1168°C / 5.7107 pp / 11.3987 ppm | Jedna trajektoria PASS; macierz 11 scenariuszy × 2 seedy: MPC **3/22 PASS, 19/22 FAIL** |
| Safety / ostatni commit | Hard-stop i interlocki dopisane na `9258ae41...` | **37/37 jednostkowych PASS** na `9258ae41`, powtórzone 37/37 po scaleniu na main; syntetyczna macierz nadal 19/22 FAIL |
| MPC D — Kaggle | R4 physics już wykonane na 2×T4 | **Duża macierz MPC NOT RUN; gate lokalny otwarty** |
| MPC E — shadow | Pasywne DTO/testy z odrzuceniem i brakiem potwierdzenia | Bez firmware i fizycznego ACK |

**Decyzja:** badawczy software gotowy do dalszej pracy; MPC nie jest zakwalifikowany dla wszystkich scenariuszy, rzeczywista wierność modelu UNKNOWN, sterowanie sprzętem BLOCKED. Najpierw [analiza 19/22 porażek — P0.2](MPC_ROADMAP.md), potem multi-hour/day E2E, **Kaggle 2×T4** i dopiero fizyczna walidacja na rzeczywistych danych.

Dokumenty szczegółowe, bez przepisywania danych:
[Fizyka i granice](THERMODYNAMIC_CORE.md) · [MPC kontrakt](MPC_RESEARCH.md) · [Quality i evidence R4](QUALITY_AND_OBSERVABILITY.md) · [Kaggle flow](KAGGLE_EXECUTION.md) · [WUR](WUR_DATASET_EVALUATION.md) · [dataset ranking](PUBLIC_DATASETS.md).

**Bezpieczeństwo:** badania wyłącznie w izolowanym `tools/ml/` na `main`, w sandbox/CPU/Kaggle. Nie modyfikować w tym torze Growclip, firmware, Rule ani OutputSupervisor. Zewnętrzna szklarnia ≠ pomiar z własnego growboxa. Nie uruchamiać dużych obliczeń bez konkretnego pytania i planu walidacji.

**Historyczny retest jest dostępny:** `agent-control:.agent/results/la-bloomml-mpc-safety-regression-20261104-v1.json` raportuje 37/37 testów jednostkowych PASS na `9258ae41` i nadal 19/22 porażek macierzy. Jest to dowód na tamten dokładny kod, nie automatycznie na wszystkie dalsze commity ani pełny build.

**Kontrola po konsolidacji:** Mac Local Agent `la-bloomml-main-consolidation-20261107-v2`, exit 0, exact `main@8ed884a56c91fcd8650796cdb3db7ea4cbf37312`, 37/37 MPC unittest i 18/18 dataset unittest PASS, badawcza gałąź usunięta. Nie zmienia to wcześniejszej oceny 19/22 FAIL ani statusu sprzętu BLOCKED.
