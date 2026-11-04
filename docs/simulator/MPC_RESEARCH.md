# BloomML — kontrakt MPC (research only)

**Aktywny status/metryki:** [kompletny checkpoint](../checkpoints/2026-11-04-bloomml-mpc-resume-complete.md). **Kolejne zadania:** [MPC_ROADMAP](MPC_ROADMAP.md). Ten dokument opisuje stabilne granice modułów, nie historię kolejnych commitów.

## Granice odpowiedzialności

- `tools/ml/physics/growbox_dynamics.py`: jedyny właściciel stanu fizyki R4; 0D, nieskalibrowany. Kontroler prognozuje na kopiach stanu, nie modyfikuje rośliny symulacyjnej podczas search.
- `tools/ml/mpc/contracts.py`: typowane nastawy T/RH **XOR** VPD/CO₂/PPFD/DLI, tolerancje, envelope, konfiguracja i statusy.
- `devices.py`: zastosowane wirtualne komendy, ograniczenia mocy/slew/dwell, conflict/hard stops, przyczyny odrzuceń, fault; `requested != applied`.
- `optimizer.py`: seeded bounded beam search / horyzont predykcji, koszt błędów klimatu, proxy energii, zużycia i przełączeń, limity ewaluacji i czasu, fallback. Deadline sprawdzany między wywołaniami fizyki: **nie jest hard realtime**.
- `lab.py`: prawdziwa symulacyjna pętla `nastawy → MPC → urządzenia → R4 → odczyty → MPC`; threshold i *stanowa* hysteresis, ślady, metryki, quality gate.
- `cli.py`: krótki/średni epizod i manifest; `scenario_matrix.py`: 11 kategorii × seedy × trzy kontrolery, w tym nieosiągalność, awarie i stale observations.
- `shadow_contract.py`: wyłącznie pasywne DTO `proposal → hypothetical Rule verdict → hypothetical OutputSupervisor response`, odrzucenia/timeout/no ACK. Zero firmware, GPIO, RF, przekaźników, sieci i zmiany uprawnień sterowania.

## Quality gates / truth

Nastawy: temperatura ±2°C, RH ±10 pp **lub** VPD ±0.3 kPa, CO₂ ±200 ppm, PPFD ±150 µmol/m²/s; DLI wymaga dopracowania periodycznego resetu. W obecnym labie `PASS` wymaga poprawnego 40-znakowego źródłowego SHA, braku fallback i safety violations, oraz ≤20% próbek poza tolerancją każdej sterowanej wielkości. Nieosiągalne żądania to FAIL albo osobno oznaczony problem feasibility, nie automatyczny PASS. `UNKNOWN != PASS`.

Trajektoria/research manifest: source SHA, seed, konfiguracja/horizon, forcing, targets, stan fizyczny R4, propozycja MPC, aplikacja wirtualnego urządzenia, fault/interlock/rejection reason, predykcja/residual/niepewność, księga wody i granice, MAE/RMSE, czas poza tolerancją, przełączenia, timeout i energia jako **nameplate proxy**. Zero podmiany estimated na measured lub wirtualnego ACK na fizyczny.

Weryfikacja R4 dotyczy stabilności numerycznej; bilans pełnego entalpicznego strumienia w układzie otwartym i osobny ledger CO₂ nadal niezamknięte. Parametry urządzeń, odpowiedzi roślin, ciśnienia, ciepła/latent i charakterystyki wyciągu **nie są skalibrowane**. Zaobserwowany in-model residual nie jest błędem wobec rzeczywistych sensorów. Brak symulacji prawdziwej zwłoki transportu/potwierdzenia wykonania i brak produkcyjnego safety certification.

## Minimalne odtworzenie

```bash
PYTHONPATH=. python -m unittest -q tests.test_mpc_devices tests.test_mpc_optimizer_mock tests.test_mpc_baselines tests.test_mpc_device_responses tests.test_mpc_physics_integration tests.test_mpc_lab_observability tests.test_mpc_shadow_contract tests.test_mpc_scenario_catalog
PYTHONPATH=. python -m tools.ml.mpc.cli --hours 1 --step-s 60 --seed 1729 --output-dir build/mpc-hourly
PYTHONPATH=. python -m tools.ml.mpc.scenario_matrix --steps 12 --step-s 60 --seeds 1729 1730 --output-dir build/mpc-matrix
```

CLI/matrix zwracają code 2 również przy ukończonym, lecz niezaliczonym gate syntetycznym; rozdziel liczbę uruchomień od jakości wyników. Zachowaj manifest i negatywne próby. Bez dokładnego `source_commit` status pozostaje UNKNOWN. Re-test ostatnich poprawek safety na `9258ae41...` to **pierwszy następny krok**; wyniki starych SHA nie dowodzą obecnej kwalifikacji.
