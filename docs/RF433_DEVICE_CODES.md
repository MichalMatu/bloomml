# RF433 device codes and growbox hardware map

Updated: 2026-09-14

Quick operational reference for learned RF433 identities. Firmware-used profiles must also be frozen in `src/climate/rf433/Rf433HardwareConfig.h` and covered by tests.

Captured remote timing and the ESP transmit profile that was physically validated are separate facts.

## Current RF433 loads

| Load | Hardware label | ON | OFF | Bits/protocol | Captured pulse | Qualified ESP TX |
| --- | --- | ---: | ---: | --- | ---: | --- |
| fan | `remote_socket_1` | `906118656` (`0x36024600`) | `1040336384` (`0x3E024600`) | 32 / 2 | 560 us | 575 us, repeat 10 |
| lamp | `remote_socket_2` | `235030016` (`0x0E024600`) | `16926208` (`0x01024600`) | 32 / 2 | 560 us | 560 us, repeat 10 |
| humidifier | `remote_socket_3` | `637683200` (`0x26024600`) | `771900928` (`0x2E024600`) | 32 / 2 | 560 us | 560 us, repeat 10 |

The fan's reliable ESP transmit timing differs from the captured remote pulse; keep `575 us / repeat 10` for that qualified profile unless new physical evidence justifies a change.

## Physical validation record

Bounded manual service-console validation was completed on 2026-09-05 with firmware `af16aebde8f69d1a1257256c7711e9721c07c9d5` while automatic runtime outputs remained fake-locked.

The operator physically confirmed ON/OFF for lamp, fan and humidifier. Local TX completion/self-RX was treated only as transport evidence, not as proof of load state.

## Service-console commands

```text
help
status
sensors
rf list
rf lamp on
rf lamp off
rf fan on
rf fan off
rf humidifier on
rf humidifier off
rf rx 1000
```

Named RF commands are explicit maintenance/service actions. They do not unlock automatic climate output ownership.

## Current production sensor topology

| Role | Source |
| --- | --- |
| inside T/RH | TP357 BLE |
| inside CO₂ | SCD41 |
| nearby/outside T/RH | Xiaomi BLE |
| time/schedule clock | DS3231/runtime clock path |

The current runtime composition intentionally uses TP357 for inside temperature/RH and only valid SCD41 CO₂ for the composed inside snapshot. See `IO_MAP.md` for the semantic boundary.

## Recording new devices

For every new RF device record:

- physical load name and neutral hardware label;
- ON/OFF decimal + hex code;
- bit length and protocol;
- captured pulse width;
- requested ESP repeat count;
- separately, the pulse/repeat combination physically proven reliable;
- exact firmware/evidence identity for the physical check.

Do not claim the original remote's repeat count unless it was measured.

## Safety boundary

Recording a code does not authorize unattended mains control. Normal configured actuation remains owned by `OutputSupervisor`; raw/named RF remains maintenance-only. Physical feedback, when needed, comes from direct observation or an independent channel such as Shelly power telemetry.
