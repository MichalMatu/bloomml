"""Scientific acceptance: stress results are not a CUDA device smoke test."""
from dataclasses import replace

import pytest
pytest.importorskip('torch')
from tools.ml.kaggle_r4_stress_runner import CATEGORIES, physics_gate, stress_cases


def test_stress_samples_all_five_mechanisms_deterministically():
    p1,s1,f1,c1=stress_cases(25,1729)
    p2,s2,f2,c2=stress_cases(25,1729)
    assert (p1,s1,f1,c1)==(p2,s2,f2,c2)
    assert set(c1)==set(CATEGORIES)
    assert f1[2].measured_pressure_difference_pa is not None
    assert p1[2].equipment.exhaust_shutoff_pressure_pa>0
    assert s1[1].wall_temperature_c < s1[1].air_temperature_c


def test_physical_gate_rejects_supersaturation_and_negative_condensate():
    p,s,f,c=stress_cases(1,1729)
    with pytest.raises(AssertionError,match='physical bounds'):
        physics_gate(p,[replace(s[0],air_vapor_kg=s[0].air_vapor_kg*10)],[0.0])
    with pytest.raises(AssertionError,match='physical bounds'):
        physics_gate(p,[replace(s[0],wall_condensate_kg=-.1)],[0.0])


def test_physical_gate_rejects_mass_residual_and_length_mismatch():
    p,s,f,c=stress_cases(1,1729)
    with pytest.raises(AssertionError,match='water conservation'):
        physics_gate(p,s,[.0001])
    with pytest.raises(AssertionError,match='length'):
        physics_gate(p,s,[])
