"""R4 numerical timestep convergence across wet/dry/pressure boundaries."""
from tools.ml.kaggle_r4_stress_runner import stress_cases
from tools.ml.physics.growbox_dynamics import GrowboxThermodynamics
from tools.ml.physics.growbox_psychrometrics import relative_humidity_pct


def test_r4_dt_convergence_across_40_heterogeneous_synthetic_boxes():
    p,s,f,categories=stress_cases(40,1729)
    worst_t=worst_rh=worst_co2=0.
    for pa,st,fo in zip(p,s,f):
        fine=GrowboxThermodynamics(pa,st); coarse=GrowboxThermodynamics(pa,st)
        for _ in range(300): fine.step(fo,1)
        for _ in range(30): coarse.step(fo,10)
        def rh(state):
            return relative_humidity_pct(state.air_temperature_c,
                                          state.air_vapor_kg/pa.geometry.air_volume_m3)
        worst_t=max(worst_t,abs(fine.state.air_temperature_c-coarse.state.air_temperature_c))
        worst_rh=max(worst_rh,abs(rh(fine.state)-rh(coarse.state)))
        worst_co2=max(worst_co2,abs(fine.state.co2_ppm-coarse.state.co2_ppm))
    assert worst_t<0.005
    assert worst_rh<0.05
    assert worst_co2<0.5
