"""Research-only, coupled growbox thermo-hygro-CO2 plant model.

Single well-mixed air zone plus wall, leaf and root/substrate temperatures.
Water is stored explicitly in vapor, soil, condensates and fog. Exchanges are
finite-volume mass fluxes; heat fluxes are in watts. This is not CFD and does
not imply an unmeasured quantity is a sensor observation.

No dependency on firmware, filesystem, ML contracts, control policy or GPU.
"""

from __future__ import annotations

import math
from dataclasses import dataclass, replace

from .growbox_convection import effective_canopy_air_speed_m_s, mixed_convection_factor
from .growbox_phase import phase_equilibrium
from .growbox_psychrometrics import (
    LATENT_HEAT_VAPORIZATION, air_vpd_kpa, dew_point_c,
    leaf_vpd_kpa, moist_air_density_kg_m3, relative_humidity_pct,
    saturation_vapor_density_kg_m3, vapor_density_kg_m3,
    vapor_pressure_pa,
)


def clip(value: float, low: float, high: float) -> float:
    return min(high, max(low, value))


def fraction(value: float) -> float:
    if not math.isfinite(value) or not 0 <= value <= 1:
        raise ValueError("actuator fraction must be finite in [0,1]")
    return value


@dataclass(frozen=True)
class GrowboxGeometry:
    air_volume_m3: float = 0.8
    wall_surface_area_m2: float = 3.0
    canopy_area_m2: float = 0.7
    wall_heat_capacity_j_k: float = 110_000.0
    leaf_heat_capacity_j_k: float = 5_000.0
    substrate_heat_capacity_j_k: float = 90_000.0
    wall_air_conductance_w_k: float = 13.0
    wall_outside_conductance_w_k: float = 7.0
    leaf_air_conductance_w_k: float = 9.0
    substrate_air_conductance_w_k: float = 2.5
    air_heat_capacity_multiplier: float = 1.0
    wall_vapor_transfer_m_s: float = 0.00015
    leaf_vapor_transfer_m_s: float = 0.00008
    bare_substrate_area_m2: float = 0.25
    substrate_evaporation_kg_m2_s: float = 0.0000007
    external_solar_area_m2: float = 0.45
    external_solar_absorptivity: float = 0.35
    reference_pressure_pa: float = 101_325.0

    def __post_init__(self) -> None:
        for name, val in vars(self).items():
            if not math.isfinite(val) or val < 0:
                raise ValueError(f"invalid geometry {name}")
        if not 50_000 <= self.reference_pressure_pa <= 110_000:
            raise ValueError("invalid reference pressure")
        if self.air_volume_m3 < 0.05 or self.canopy_area_m2 <= 0:
            raise ValueError("growbox air volume and canopy area must be positive")
        if min(self.wall_heat_capacity_j_k, self.leaf_heat_capacity_j_k,
               self.substrate_heat_capacity_j_k, self.air_heat_capacity_multiplier) <= 0:
            raise ValueError("heat capacities and multiplier must be positive")


@dataclass(frozen=True)
class GrowboxLighting:
    rated_power_w: float = 120.0
    # Electrical heat released inside box; e.g. remotely mounted driver can reduce this.
    in_box_fraction: float = 1.0
    air_fraction: float = 0.45
    leaf_fraction: float = 0.30
    wall_fraction: float = 0.25
    photon_efficacy_umol_j: float = 2.3
    canopy_capture_fraction: float = 0.65
    solar_window_area_m2: float = 0.0
    solar_canopy_fraction: float = 0.4

    def __post_init__(self) -> None:
        for name, val in vars(self).items():
            if not math.isfinite(val) or val < 0:
                raise ValueError(f"invalid light parameter {name}")
        for name in ("in_box_fraction", "air_fraction", "leaf_fraction", "wall_fraction", "canopy_capture_fraction", "solar_canopy_fraction"):
            if getattr(self, name) > 1:
                raise ValueError(f"invalid fraction {name}")
        if not math.isclose(self.air_fraction + self.leaf_fraction + self.wall_fraction, 1.0, abs_tol=1e-9):
            raise ValueError("lamp energy partitions must total 1")


@dataclass(frozen=True)
class GrowboxBiology:
    substrate_water_capacity_kg: float = 3.0
    maximum_transpiration_g_h: float = 85.0
    light_half_saturation_ppfd: float = 200.0
    vpd_half_saturation_kpa: float = 0.5
    stomatal_closure_vpd_kpa: float = 2.7
    transpiration_dark_fraction: float = 0.06
    maximum_co2_assimilation_umol_m2_s: float = 12.0
    respiration_umol_m2_s: float = 1.0
    co2_half_saturation_ppm: float = 350.0
    photosynthesis_optimal_leaf_temperature_c: float = 25.0
    photosynthesis_temperature_width_c: float = 12.0

    def __post_init__(self) -> None:
        for name, val in vars(self).items():
            if not math.isfinite(val) or val < 0:
                raise ValueError(f"invalid plant parameter {name}")
        if self.substrate_water_capacity_kg <= 0 or self.light_half_saturation_ppfd <= 0 or self.vpd_half_saturation_kpa <= 0 or self.stomatal_closure_vpd_kpa <= 0 or self.co2_half_saturation_ppm <= 0:
            raise ValueError("invalid plant saturation constant")
        if self.photosynthesis_temperature_width_c <= 0:
            raise ValueError("photosynthesis temperature width must be >0")
        if self.transpiration_dark_fraction > 1:
            raise ValueError("invalid dark transpiration fraction")


@dataclass(frozen=True)
class GrowboxEquipment:
    heater_power_w: float = 180.0
    cooler_capacity_w: float = 200.0
    exhaust_max_m3_h: float = 90.0
    # Optional characterized fan shutoff pressure; zero disables fan curve.
    exhaust_shutoff_pressure_pa: float = 0.0
    infiltration_ach: float = 0.25
    humidifier_output_g_h: float = 110.0
    humidifier_air_cooling_fraction: float = 0.3
    dehumidifier_removal_g_h: float = 80.0
    dehumidifier_heat_w: float = 45.0
    dehumidifier_latent_return_fraction: float = 0.7
    co2_injection_g_h: float = 25.0
    fan_electrical_heat_w: float = 5.0
    # Internal circulation changes local heat and mass transport, NOT outside ACH.
    circulation_fan_heat_w: float = 3.0
    background_air_speed_m_s: float = 0.10
    exhaust_induced_air_speed_m_s: float = 0.20
    circulation_reference_air_speed_m_s: float = 0.60
    cooler_coil_condensation_kg_s_k: float = 0.000005
    cooler_coil_depression_c: float = 10.0

    def __post_init__(self) -> None:
        for name, val in vars(self).items():
            if not math.isfinite(val) or val < 0:
                raise ValueError(f"invalid equipment parameter {name}")
        if self.dehumidifier_latent_return_fraction > 1:
            raise ValueError("invalid latent return fraction")
        if self.humidifier_air_cooling_fraction > 1:
            raise ValueError("invalid humidifier cooling fraction")


@dataclass(frozen=True)
class GrowboxPhysics:
    geometry: GrowboxGeometry = GrowboxGeometry()
    lighting: GrowboxLighting = GrowboxLighting()
    biology: GrowboxBiology = GrowboxBiology()
    equipment: GrowboxEquipment = GrowboxEquipment()


@dataclass(frozen=True)
class GrowboxForcing:
    outside_temperature_c: float = 20.0
    outside_relative_humidity_pct: float = 50.0
    outside_co2_ppm: float = 420.0
    atmospheric_pressure_pa: float = 101_325.0
    # Used as measured effective airflow if present; otherwise nameplate-flow model.
    measured_exhaust_m3_h: float | None = None
    measured_air_speed_m_s: float | None = None
    measured_pressure_difference_pa: float | None = None
    lamp_dimmer: float = 0.0
    lamp_measured_electrical_w: float | None = None
    canopy_measured_ppfd_umol_m2_s: float | None = None
    solar_irradiance_w_m2: float = 0.0
    solar_measured_ppfd_umol_m2_s: float = 0.0
    heater: float = 0.0
    cooler: float = 0.0
    exhaust_fan: float = 0.0
    circulation_fan: float = 0.0
    humidifier: float = 0.0
    dehumidifier: float = 0.0
    co2_doser: float = 0.0
    irrigation_kg_s: float = 0.0

    def validate(self) -> None:
        for name in ("lamp_dimmer", "heater", "cooler", "exhaust_fan", "circulation_fan", "humidifier", "dehumidifier", "co2_doser"):
            fraction(getattr(self, name))
        if not -40 <= self.outside_temperature_c <= 80 or not 0 <= self.outside_relative_humidity_pct <= 100:
            raise ValueError("outside boundary is not physical")
        for name in ("outside_co2_ppm", "solar_irradiance_w_m2", "solar_measured_ppfd_umol_m2_s", "irrigation_kg_s"):
            value = getattr(self, name)
            if not math.isfinite(value) or value < 0:
                raise ValueError(f"invalid forcing {name}")
        for name in ("measured_exhaust_m3_h", "measured_air_speed_m_s", "lamp_measured_electrical_w", "canopy_measured_ppfd_umol_m2_s"):
            value = getattr(self, name)
            if value is not None and (not math.isfinite(value) or value < 0):
                raise ValueError(f"invalid measured forcing {name}")
        if self.measured_pressure_difference_pa is not None:
            if not math.isfinite(self.measured_pressure_difference_pa) or abs(self.measured_pressure_difference_pa) > 5000:
                raise ValueError("invalid measured pressure difference")
        moist_air_density_kg_m3(20, self.atmospheric_pressure_pa, 0)


@dataclass(frozen=True)
class GrowboxState:
    air_temperature_c: float
    wall_temperature_c: float
    leaf_temperature_c: float
    substrate_temperature_c: float
    air_vapor_kg: float
    co2_ppm: float
    substrate_water_kg: float
    wall_condensate_kg: float = 0.0
    leaf_condensate_kg: float = 0.0
    cooler_condensate_kg: float = 0.0
    dehumidifier_condensate_kg: float = 0.0
    drainage_kg: float = 0.0
    fog_condensate_kg: float = 0.0
    # Signed net water influx through the boundary, for rigorous water accounting.
    cumulative_net_external_water_kg: float = 0.0
    cumulative_added_water_kg: float = 0.0
    elapsed_s: float = 0.0
    cumulative_dli_mol_m2: float = 0.0

    @classmethod
    def initial(cls, *, air_temperature_c: float = 24, rh_pct: float = 60,
                co2_ppm: float = 700, volume_m3: float = 0.8,
                wall_temperature_c: float | None = None,
                leaf_temperature_c: float | None = None,
                substrate_temperature_c: float | None = None,
                substrate_water_kg: float = 1.5) -> "GrowboxState":
        if not 0 < volume_m3 or not 0 <= co2_ppm <= 100000:
            raise ValueError("invalid initial box")
        return cls(air_temperature_c=air_temperature_c,
                   wall_temperature_c=air_temperature_c if wall_temperature_c is None else wall_temperature_c,
                   leaf_temperature_c=air_temperature_c if leaf_temperature_c is None else leaf_temperature_c,
                   substrate_temperature_c=air_temperature_c if substrate_temperature_c is None else substrate_temperature_c,
                   air_vapor_kg=vapor_density_kg_m3(air_temperature_c, rh_pct) * volume_m3,
                   co2_ppm=co2_ppm, substrate_water_kg=substrate_water_kg)

    def total_water_kg(self) -> float:
        return (self.air_vapor_kg + self.substrate_water_kg + self.wall_condensate_kg
                + self.leaf_condensate_kg + self.drainage_kg
                + self.cooler_condensate_kg + self.dehumidifier_condensate_kg
                + self.fog_condensate_kg)


@dataclass(frozen=True)
class GrowboxObservables:
    rh_pct: float
    dew_point_c: float | None
    air_vpd_kpa: float
    leaf_vpd_kpa: float
    ppfd_umol_m2_s: float
    lamp_power_w: float
    air_density_kg_m3: float
    exhaust_m3_h: float
    exhaust_source: str
    canopy_air_speed_m_s: float
    canopy_air_speed_source: str
    wall_condensation_risk: bool
    leaf_condensation_risk: bool
    dli_mol_m2: float
    partial_vapor_pressure_pa: float
    measured_pressure_difference_pa: float | None
    air_vapor_kg: float


class GrowboxThermodynamics:
    """Single owner of physical state; no firmware/control/measurement mutations."""

    def __init__(self, parameters: GrowboxPhysics, state: GrowboxState):
        self.parameters = parameters
        self.state = state
        if state.substrate_water_kg > parameters.biology.substrate_water_capacity_kg:
            raise ValueError("initial soil water exceeds configured capacity")
        self.initial_water_kg = state.total_water_kg() - state.cumulative_net_external_water_kg - state.cumulative_added_water_kg
        # Fixed reference air thermal inertia. Re-evaluating rho(T) each step
        # silently creates/removes sensible energy in an isolated constant-volume
        # chamber; ventilation changes enthalpy separately through heat_vent_w.
        rho_v_initial = state.air_vapor_kg / parameters.geometry.air_volume_m3
        self._reference_air_density_kg_m3 = moist_air_density_kg_m3(
            state.air_temperature_c, parameters.geometry.reference_pressure_pa,
            vapor_pressure_pa(state.air_temperature_c, rho_v_initial),
        )

    def _lamp(self, forcing: GrowboxForcing) -> tuple[float, float]:
        lamp_w = (self.parameters.lighting.rated_power_w * forcing.lamp_dimmer
                  if forcing.lamp_measured_electrical_w is None
                  else forcing.lamp_measured_electrical_w)
        light = self.parameters.lighting
        ppfd = (lamp_w * light.photon_efficacy_umol_j * light.canopy_capture_fraction
                / self.parameters.geometry.canopy_area_m2
                if forcing.canopy_measured_ppfd_umol_m2_s is None
                else forcing.canopy_measured_ppfd_umol_m2_s)
        return lamp_w, ppfd + forcing.solar_measured_ppfd_umol_m2_s

    def _exhaust(self, forcing: GrowboxForcing) -> float:
        device = self.parameters.equipment
        if forcing.measured_exhaust_m3_h is not None:
            return forcing.measured_exhaust_m3_h
        ideal = device.exhaust_max_m3_h * forcing.exhaust_fan
        if (device.exhaust_shutoff_pressure_pa > 0
                and forcing.measured_pressure_difference_pa is not None
                and forcing.exhaust_fan > 0):
            # Quadratic fan curve; pressure follows affinity law (speed^2).
            pressure_limit = device.exhaust_shutoff_pressure_pa * forcing.exhaust_fan ** 2
            return ideal * math.sqrt(max(0.0, 1.0 - abs(forcing.measured_pressure_difference_pa)
                                         / pressure_limit))
        return ideal

    def _exhaust_source(self, forcing: GrowboxForcing) -> str:
        if forcing.measured_exhaust_m3_h is not None:
            return 'measured'
        if (self.parameters.equipment.exhaust_shutoff_pressure_pa > 0
                and forcing.measured_pressure_difference_pa is not None):
            return 'characterized_curve_estimate'
        return 'nameplate_estimate'

    def _canopy_air_speed(self, forcing: GrowboxForcing) -> tuple[float, str]:
        eq = self.parameters.equipment
        return effective_canopy_air_speed_m_s(
            measured_air_speed_m_s=forcing.measured_air_speed_m_s,
            exhaust_command=forcing.exhaust_fan,
            circulation_command=forcing.circulation_fan,
            background_air_speed_m_s=eq.background_air_speed_m_s,
            exhaust_induced_air_speed_m_s=eq.exhaust_induced_air_speed_m_s,
            circulation_air_speed_m_s=eq.circulation_reference_air_speed_m_s,
        )

    def observe(self, forcing: GrowboxForcing) -> GrowboxObservables:
        forcing.validate()
        s = self.state
        v = self.parameters.geometry.air_volume_m3
        partial_pa = vapor_pressure_pa(s.air_temperature_c, s.air_vapor_kg / v)
        lamp_w, ppfd = self._lamp(forcing)
        air_speed, speed_source = self._canopy_air_speed(forcing)
        return GrowboxObservables(
            rh_pct=relative_humidity_pct(s.air_temperature_c, s.air_vapor_kg / v),
            dew_point_c=dew_point_c(partial_pa),
            air_vpd_kpa=air_vpd_kpa(s.air_temperature_c, partial_pa),
            leaf_vpd_kpa=leaf_vpd_kpa(s.leaf_temperature_c, partial_pa),
            ppfd_umol_m2_s=ppfd, lamp_power_w=lamp_w,
            air_density_kg_m3=moist_air_density_kg_m3(
                s.air_temperature_c, forcing.atmospheric_pressure_pa, partial_pa),
            exhaust_m3_h=self._exhaust(forcing),
            exhaust_source=self._exhaust_source(forcing),
            canopy_air_speed_m_s=air_speed,
            canopy_air_speed_source=speed_source,
            wall_condensation_risk=(s.wall_temperature_c < (dew_point_c(partial_pa) or -1000.0)),
            leaf_condensation_risk=(s.leaf_temperature_c < (dew_point_c(partial_pa) or -1000.0)),
            dli_mol_m2=s.cumulative_dli_mol_m2,
            partial_vapor_pressure_pa=partial_pa,
            measured_pressure_difference_pa=forcing.measured_pressure_difference_pa,
            air_vapor_kg=s.air_vapor_kg,
        )

    def step(self, forcing: GrowboxForcing, dt_s: float) -> GrowboxState:
        forcing.validate()
        if not math.isfinite(dt_s) or not 0 < dt_s <= 3600:
            raise ValueError("timestep must be positive and <= one hour")
        g = self.parameters.geometry
        if not -40 <= self.state.air_temperature_c <= 80:
            raise ValueError("air outside modeled temperature range")
        # Keep explicit flux integration stable even for strong box fan + small V.
        exchange_s = (self._exhaust(forcing) / 3600.0 / g.air_volume_m3
                      + self.parameters.equipment.infiltration_ach / 3600)
        # Fixed C_air with worst-case capped mixed-convection transfer prevents
        # explicit-integration instability under strong internal circulation.
        c_air = self._reference_air_density_kg_m3 * g.air_volume_m3 * 1005.0 * g.air_heat_capacity_multiplier
        air_ua_max = 4 * (g.leaf_air_conductance_w_k + g.wall_air_conductance_w_k
                          + g.substrate_air_conductance_w_k)
        characteristic_s = min(
            2.0,
            0.05 * c_air / max(1e-12, air_ua_max),
            0.05 / max(1.0e-12, exchange_s),
            0.05 * g.wall_heat_capacity_j_k / max(1.0e-12, 4.0 * g.wall_air_conductance_w_k + g.wall_outside_conductance_w_k),
            0.05 * g.leaf_heat_capacity_j_k / max(1.0e-12, 4.0 * g.leaf_air_conductance_w_k),
            0.05 * g.substrate_heat_capacity_j_k / max(1.0e-12, 4.0 * g.substrate_air_conductance_w_k),
        )
        steps = max(1, math.ceil(dt_s / characteristic_s))
        if steps > 30_000:
            raise ValueError("physics timestep requires excessive substeps; revise geometry/flow")
        h = dt_s / steps
        # A failed late substep must not publish a partially advanced state.
        candidate = self.state
        for _ in range(steps):
            candidate = self._substep(forcing, h, candidate)
        self.state = candidate
        return candidate

    def _substep(self, forcing: GrowboxForcing, h: float, s: GrowboxState) -> GrowboxState:
        g, eq, bio, light = (self.parameters.geometry, self.parameters.equipment,
                             self.parameters.biology, self.parameters.lighting)
        v = g.air_volume_m3
        speed, _speed_source = self._canopy_air_speed(forcing)
        leaf_factor = mixed_convection_factor(
            speed, s.leaf_temperature_c, s.air_temperature_c,
            characteristic_length_m=0.10,
        )
        wall_factor = mixed_convection_factor(
            speed, s.wall_temperature_c, s.air_temperature_c,
            characteristic_length_m=0.80,
        )
        substrate_factor = mixed_convection_factor(
            speed, s.substrate_temperature_c, s.air_temperature_c,
            characteristic_length_m=0.40,
        )
        lamp_w, ppfd = self._lamp(forcing)
        lamp_box_w = lamp_w * light.in_box_fraction
        solar_box_w = forcing.solar_irradiance_w_m2 * light.solar_window_area_m2
        solar_external_wall_w = (forcing.solar_irradiance_w_m2
                                 * g.external_solar_area_m2 * g.external_solar_absorptivity)
        light_air_w = lamp_box_w * light.air_fraction
        light_leaf_w = lamp_box_w * light.leaf_fraction + solar_box_w * light.solar_canopy_fraction
        light_wall_w = (lamp_box_w * light.wall_fraction
                        + solar_box_w * (1.0 - light.solar_canopy_fraction)
                        + solar_external_wall_w)

        vapor_density = s.air_vapor_kg / v
        partial_pa = vapor_pressure_pa(s.air_temperature_c, vapor_density)
        density = moist_air_density_kg_m3(s.air_temperature_c, forcing.atmospheric_pressure_pa, partial_pa)
        air_capacity = self._reference_air_density_kg_m3 * v * 1005.0 * g.air_heat_capacity_multiplier
        outside_vapor_density = vapor_density_kg_m3(forcing.outside_temperature_c,
                                                    forcing.outside_relative_humidity_pct)
        q_m3_s = (self._exhaust(forcing) / 3600.0 + eq.infiltration_ach * v / 3600.0)
        water_vent_kg = q_m3_s * (outside_vapor_density - vapor_density) * h
        # Sensible moist-air exchange: separate dry-air and vapor capacities.
        # Vapor latent enthalpy follows transported vapor; do not double count.
        rho_dry_air = max(0.0, (forcing.atmospheric_pressure_pa - partial_pa)
                              / (287.05 * (s.air_temperature_c + 273.15)))
        sensible_capacity_per_m3 = rho_dry_air * 1005.0 + vapor_density * 1860.0
        heat_vent_w = sensible_capacity_per_m3 * q_m3_s * (forcing.outside_temperature_c - s.air_temperature_c)

        # Wall condensation is controlled by surface saturation and may re-evaporate.
        saturation_wall = saturation_vapor_density_kg_m3(s.wall_temperature_c)
        wall_flux_kg = (g.wall_vapor_transfer_m_s * wall_factor * g.wall_surface_area_m2
                        * (vapor_density - saturation_wall) * h)
        if wall_flux_kg > 0:
            wall_flux_kg = min(wall_flux_kg, s.air_vapor_kg)
        else:
            wall_flux_kg = -min(-wall_flux_kg, s.wall_condensate_kg)

        # Leaf wetness: cold leaf surfaces condense water, then re-evaporate as
        # leaf warms. This condensation is distinct from biological transpiration.
        saturation_leaf = saturation_vapor_density_kg_m3(s.leaf_temperature_c)
        leaf_flux_kg = (g.leaf_vapor_transfer_m_s * leaf_factor * g.canopy_area_m2
                        * (vapor_density - saturation_leaf) * h)
        if leaf_flux_kg > 0:
            leaf_flux_kg = min(leaf_flux_kg, s.air_vapor_kg)
        else:
            leaf_flux_kg = -min(-leaf_flux_kg, s.leaf_condensate_kg)

        # Bare substrate evaporation transfers liquid water and sensible energy
        # without creating water from nothing.
        soil_rh_surface = clip(s.substrate_water_kg / bio.substrate_water_capacity_kg * 2.5, 0, 1)
        soil_vapour_deficit = max(0.0, saturation_vapor_density_kg_m3(s.substrate_temperature_c)
                                   * soil_rh_surface - vapor_density)
        soil_evap_kg = min(s.substrate_water_kg,
                           g.substrate_evaporation_kg_m2_s * g.bare_substrate_area_m2
                           * soil_vapour_deficit / 0.01 * substrate_factor * h)

        # Photosynthesis and transpiration depend on effective light, leaf VPD,
        # carbon availability and remaining substrate water.
        leaf_vpd = max(0.0, leaf_vpd_kpa(s.leaf_temperature_c, partial_pa))
        light_response = ppfd / (ppfd + bio.light_half_saturation_ppfd)
        stomata = bio.transpiration_dark_fraction + (1.0 - bio.transpiration_dark_fraction) * light_response
        vpd_response = (leaf_vpd / (leaf_vpd + bio.vpd_half_saturation_kpa)
                        / (1.0 + (leaf_vpd / bio.stomatal_closure_vpd_kpa) ** 2))
        soil_response = clip(s.substrate_water_kg / (0.3 * bio.substrate_water_capacity_kg), 0, 1)
        transp_kg = min(max(0.0, s.substrate_water_kg - soil_evap_kg),
                        bio.maximum_transpiration_g_h / 3_600_000.0
                        * stomata * vpd_response * soil_response
                        * max(0.0, leaf_factor) ** 0.25 * h)

        # Air humidity and condensation; assume ultrasonic/evaporative humidifier
        # extracts specified fraction of latent heat from air.
        humidifier_kg = eq.humidifier_output_g_h / 3_600_000.0 * forcing.humidifier * h
        dehumidifier_kg = min(s.air_vapor_kg, eq.dehumidifier_removal_g_h / 3_600_000.0 * forcing.dehumidifier * h)
        cooler_coil_t = s.air_temperature_c - eq.cooler_coil_depression_c
        cooler_condensate = 0.0
        if forcing.cooler > 0 and cooler_coil_t >= -40:
            cooler_condensate = (forcing.cooler * eq.cooler_coil_condensation_kg_s_k
                                  * max(0.0, vapor_density - saturation_vapor_density_kg_m3(cooler_coil_t))
                                  / max(1.0e-6, saturation_vapor_density_kg_m3(cooler_coil_t))
                                  * h)
            cooler_condensate = min(cooler_condensate, s.air_vapor_kg)

        # Strong air cooling humidification and high vapor loads eventually form fog.
        # Finite water budget: cannot remove more water than available after additions.
        available = max(0.0, s.air_vapor_kg + water_vent_kg + humidifier_kg + transp_kg + soil_evap_kg)
        total_out = max(0.0, wall_flux_kg) + max(0.0, leaf_flux_kg) + dehumidifier_kg + cooler_condensate
        if total_out > available:
            scale = available / total_out
            wall_flux_kg = max(0.0, wall_flux_kg) * scale
            leaf_flux_kg = max(0.0, leaf_flux_kg) * scale
            dehumidifier_kg *= scale
            cooler_condensate *= scale
        vapor_new = max(0, s.air_vapor_kg + water_vent_kg + humidifier_kg + transp_kg
                        + soil_evap_kg - wall_flux_kg - leaf_flux_kg
                        - dehumidifier_kg - cooler_condensate)
        # Sensible heat plus latent flows. Wall condensation heats walls, leaf
        # transpiration cools leaves; cold-coil condensate consumes cooling power.
        leaf_air_w = (g.leaf_air_conductance_w_k * leaf_factor
                      * (s.leaf_temperature_c - s.air_temperature_c))
        wall_air_w = g.wall_air_conductance_w_k * wall_factor * (s.wall_temperature_c - s.air_temperature_c)
        substrate_air_w = g.substrate_air_conductance_w_k * substrate_factor * (s.substrate_temperature_c - s.air_temperature_c)
        cooler_w = eq.cooler_capacity_w * forcing.cooler
        latent_coil_w = min(cooler_w, cooler_condensate / h * LATENT_HEAT_VAPORIZATION)
        air_dT = ((eq.heater_power_w * forcing.heater + light_air_w
                   + eq.fan_electrical_heat_w * forcing.exhaust_fan
                   + eq.circulation_fan_heat_w * forcing.circulation_fan
                   + eq.dehumidifier_heat_w * forcing.dehumidifier
                   + dehumidifier_kg / h * LATENT_HEAT_VAPORIZATION
                     * eq.dehumidifier_latent_return_fraction
                   - cooler_w + latent_coil_w
                   - humidifier_kg / h * LATENT_HEAT_VAPORIZATION * eq.humidifier_air_cooling_fraction
                   + heat_vent_w + leaf_air_w + wall_air_w + substrate_air_w) * h / air_capacity)
        leaf_dT = ((light_leaf_w - leaf_air_w
                     + leaf_flux_kg / h * LATENT_HEAT_VAPORIZATION
                     - transp_kg / h * LATENT_HEAT_VAPORIZATION) * h
                   / g.leaf_heat_capacity_j_k)
        wall_dT = ((light_wall_w - wall_air_w
                    + g.wall_outside_conductance_w_k * (forcing.outside_temperature_c - s.wall_temperature_c)
                    + wall_flux_kg / h * LATENT_HEAT_VAPORIZATION) * h
                   / g.wall_heat_capacity_j_k)
        substrate_dT = ((-substrate_air_w - soil_evap_kg / h * LATENT_HEAT_VAPORIZATION)
                         * h / g.substrate_heat_capacity_j_k)
        t_air = s.air_temperature_c + air_dT
        t_wall = s.wall_temperature_c + wall_dT
        t_leaf = s.leaf_temperature_c + leaf_dT
        t_sub = s.substrate_temperature_c + substrate_dT

        # Saturation at final T releases/consumes corresponding latent heat.
        # Fog evaporation is per elapsed time, not per numerical substep.
        vapor_new, fog, t_air = phase_equilibrium(
            vapor_kg=vapor_new, fog_kg=s.fog_condensate_kg,
            air_temperature_c=t_air, volume_m3=v,
            air_capacity_j_k=air_capacity, dt_s=h,
        )
        # CO2: well-mixed supply, outside exchange, plant carbon uptake/respiration.
        mol_air = forcing.atmospheric_pressure_pa * v / (8.314462618 * (s.air_temperature_c + 273.15))
        co2_injection_ppm_s = ((eq.co2_injection_g_h / 3600.0 * forcing.co2_doser / 44.01)
                               * 1_000_000.0 / mol_air)
        phot_umol_s = (bio.maximum_co2_assimilation_umol_m2_s * g.canopy_area_m2
                       * light_response * s.co2_ppm / (s.co2_ppm + bio.co2_half_saturation_ppm)
                       * math.exp(-0.5 * ((s.leaf_temperature_c
                           - bio.photosynthesis_optimal_leaf_temperature_c)
                           / bio.photosynthesis_temperature_width_c) ** 2)
                       * soil_response)
        respiration_umol_s = bio.respiration_umol_m2_s * g.canopy_area_m2
        biology_ppm_s = (respiration_umol_s - phot_umol_s) / mol_air
        co2_new = max(0, s.co2_ppm + (q_m3_s / v * (forcing.outside_co2_ppm - s.co2_ppm)
                                     + co2_injection_ppm_s + biology_ppm_s) * h)

        soil_water = min(bio.substrate_water_capacity_kg,
                         s.substrate_water_kg + forcing.irrigation_kg_s * h - transp_kg - soil_evap_kg)
        overflow = max(0.0, s.substrate_water_kg + forcing.irrigation_kg_s * h - transp_kg - soil_evap_kg - bio.substrate_water_capacity_kg)
        # Overflow leaves through drainage; account as accumulated liquid output.
        result = replace(s,
                         air_temperature_c=t_air, wall_temperature_c=t_wall,
                         leaf_temperature_c=t_leaf, substrate_temperature_c=t_sub,
                         air_vapor_kg=vapor_new, co2_ppm=co2_new,
                         substrate_water_kg=soil_water,
                         wall_condensate_kg=s.wall_condensate_kg + wall_flux_kg,
                         leaf_condensate_kg=s.leaf_condensate_kg + leaf_flux_kg,
                         cooler_condensate_kg=s.cooler_condensate_kg + cooler_condensate,
                         dehumidifier_condensate_kg=s.dehumidifier_condensate_kg + dehumidifier_kg,
                         drainage_kg=s.drainage_kg + overflow,
                         fog_condensate_kg=fog,
                         cumulative_net_external_water_kg=s.cumulative_net_external_water_kg + water_vent_kg,
                         cumulative_added_water_kg=s.cumulative_added_water_kg + humidifier_kg + forcing.irrigation_kg_s * h,
                         elapsed_s=s.elapsed_s + h,
                         cumulative_dli_mol_m2=s.cumulative_dli_mol_m2 + ppfd * h * 1e-6)
        for name in ("air_temperature_c", "wall_temperature_c", "leaf_temperature_c", "substrate_temperature_c"):
            if not math.isfinite(getattr(result, name)) or not -40 <= getattr(result, name) <= 80:
                raise ValueError(f"physical state diverged: {name}")
        return result

    def water_balance_residual_kg(self) -> float:
        s = self.state
        return (s.total_water_kg() - s.cumulative_net_external_water_kg
                - s.cumulative_added_water_kg - self.initial_water_kg)
