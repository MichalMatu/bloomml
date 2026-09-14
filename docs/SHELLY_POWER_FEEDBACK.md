# Shelly power-feedback reference

Updated: 2026-09-14

## Device/reference state

The growbox test setup has used a Shelly Plug S Gen3 at `http://192.168.0.16` as an independent mains-power observation channel.

Observed during qualification:

- model `S3PL-00112EU` / Plug S Gen3;
- firmware `1.7.5` at the time of the test;
- RPC authentication was disabled at that time.

Do not assume network reachability, firmware or authentication state is unchanged; re-read device identity before a new hardware qualification.

Useful RPC reads:

```sh
curl -fsS --max-time 5 http://192.168.0.16/rpc/Shelly.GetDeviceInfo
curl -fsS --max-time 5 'http://192.168.0.16/rpc/Switch.GetStatus?id=0'
```

Important status fields: `output`, `apower`, `voltage`, `current`, `aenergy.total`, `temperature.tC`.

For an explicitly authorized relay write use deterministic `Switch.Set`, then read status back. Do not use `Switch.Toggle` in automation.

## Why it is useful

Shelly is independent of the ESP32/RF433 transmit path, so a before/after active-power delta can provide physical evidence that a mains load changed state.

Model the observation as:

```text
requested state
-> RF command
-> expected load power signature
-> Shelly measured delta
-> physical-state confidence / anomaly
```

RF TX completion alone is not physical acknowledgement.

## Calibrated reference signatures

Two supervised calibrations on 2026-09-05, including a 20-second settled repeat, produced these useful centers/ranges:

| Load | Approx. power contribution |
| --- | ---: |
| lamp | 97.0-97.1 W |
| exhaust fan | 2.8-3.2 W |
| humidifier | 15.4-15.7 W |
| all controlled loads OFF baseline | about 2.2 W |

Observed mains during those tests was roughly 243-245 V.

These are reference signatures, not immutable acceptance constants. Use multiple samples and a median/trimmed estimate, record mains voltage, and reject ambiguous windows where another load changes.

## Recommended confirmation sequence

For a supervised actuator check:

1. read a stable pre-transition Shelly window;
2. issue one explicit actuator transition;
3. allow settling;
4. read a stable post-transition window;
5. compare the median delta against the calibrated range/tolerance;
6. perform the reverse transition and confirm the opposite-sign delta;
7. end in the intended safe state and record final power.

Do not switch multiple loads in one calibration transition if you need attribution.

## Safety/master-relay role

Shelly is not the normal climate-control owner and is not the normal thermal response mechanism.

Normal lamp overtemperature handling remains actuator-specific: lamp OFF while exhaust fan cooling remains available. A master cutoff could remove power from cooling or the controller, so automatic Shelly master-off behavior requires a separately documented and physically qualified safety design.

Use Shelly writes only in explicitly authorized supervised work until such a design exists.

## Evidence boundary

The calibration demonstrates that these three RF loads produced distinct independently measured power deltas. It does not prove future commands succeeded, and it does not transfer output ownership away from `OutputSupervisor`.
