# BloomML — Kaggle 2×Tesla T4 / Local Agent

**Aktualny stan i roadmapa:** [RESEARCH_STATUS.md](RESEARCH_STATUS.md). **Jakość i logi:** [QUALITY_AND_OBSERVABILITY.md](QUALITY_AND_OBSERVABILITY.md).

## Co jest połączone

`GitHub (research branch) → Mac Local Agent → prywatny notebook Kaggle → zapisany manifest → Local Agent → ocena numeryczna`. Token Kaggle pozostaje **lokalnie na Macu**. Nie wpisuj go w kod, notebook lub czat. To wywołania API **na żądanie**, nie stała sesja GPU i nie automatyczny trigger GitHub.

Prywatny sprawdzony notebook: https://www.kaggle.com/code/meeehooow/bloomml-r4-physics-stress . Notebook źródłowy: `notebooks/BloomML_Kaggle_R4_Stress.ipynb`; uruchamia `tools/ml/kaggle_r4_stress_runner.py` na `cuda:0` i `cuda:1`. Obie karty wymagane, bez udawania powodzenia przez fallback CPU. Kaggle może ograniczać czas, quota i dostępność GPU.

## Odtworzenie i wynik

1. Sprawdź `AGENTS.md`, aktualny SHA `main` i branch `research/simulator-fidelity-kaggle-20261103`, potem `agent-control:.agent/status/daemon.json`. Nie zakładaj, że aktualny HEAD jest dawnym SHA notebooka.
2. W Kaggle używaj **Private**, Accelerator **GPU T4 ×2**, Internet ON do klonowania. Nie publikuj logów z fizycznych czujników lub sekretów. Dla ścisłej reprodukcji checkout dokładnego `source_commit` z manifestu, nie wyłącznie najnowszy HEAD.
3. Mac Local Agent ma uwierzytelnione Kaggle CLI. Przykładowa kontrola (bez sekretów):

```bash
kaggle kernels status meeehooow/bloomml-r4-physics-stress
mkdir -p /tmp/bloomml-kaggle-check
kaggle kernels output meeehooow/bloomml-r4-physics-stress \
  --file-pattern 'r4_stress_manifest' -p /tmp/bloomml-kaggle-check
```

4. Manifest zwykle w podkatalogu `bloomml-r4/r4_stress_manifest.json`. Nie pobieraj pełnego snapshotu repozytorium — `--file-pattern` ma ograniczać pobranie do manifestu. Sprawdź identyczny SHA kodu, obie karty, sumy scenario ticks, błędy parity, bilans wody i alerty RH/CO₂:

```bash
PYTHONPATH=. python -m tools.ml.research_evidence \
  /tmp/bloomml-kaggle-check/bloomml-r4/r4_stress_manifest.json --root .
```

## Zweryfikowane runy — nie trzeba ich powtarzać

- R1 CPU oracle + test obu CUDA: wcześniejszy notebook. R2 rzeczywisty pilot: `84ee2245550b095422a9e5dd6c74834bd1ecc8f4`, 512 komór, `COMPLETE`.
- R4 V1: `c9e5d2e37e02aa345d25606120416b357175d296`, 8192 komór, 24,42 mln podkroków; wykrył sztuczne osuszanie.
- **R4 V2: `05953e0139cfc31c7ba1fde6ba181d6a90bb9fb3`**, takie samo obciążenie po naprawie; obie T4 `COMPLETE`, minimum RH 6,65%, |water residual| max 3,8e-14 kg, 25 przypadków CO₂>2000 ppm. Szczegóły: [R4 checkpoint](../checkpoints/2026-11-04-r4-dual-t4-stress.md), wynik zadania: `agent-control:.agent/results/la-bloomml-r4-corrected-kaggle-verify-20261104-v2.json`.

**Uwaga naukowa:** test GPU, zgodność CPU i fizyczne ograniczenia **nie dowodzą** wierności rzeczywistej szklarni/growboxa. Nie raportujemy przyspieszenia CPU↔GPU bez uczciwego benchmarku na tym samym sprzęcie, ani kwalifikacji modelu ML bez realnego treningu/holdoutu. `main`, produkcyjna automatyka i Growclip bez zmian.
