# BloomML — kanoniczna mapa konfiguracji IPO v2

**Stan: browser DRAFT; host qualification PASS; CrowPanel firmware BUILD PASS; physical apply CLOSED.** Ten dokument opisuje **bieżący kontrakt**, a nie historyczne etapy Bridge 1–4/B5. Historia i dokładne SHA są w [checkpointach](checkpoints/2026-11-07-bloomml-documentation-freeze-handoff.md).

## Co jest aktywne, a co nie

```text
Konfigurator / (DeviceProfile v2, offline JSON)
  DeviceRegistry -> SignalRegistry -> ProcessRegistry
      -> OutputRegistry -> ProcessBindings -> Board Readiness
  [DRAFT / EXPORT / PREVIEW ONLY, no device authorization]
                         |
                no live endpoint
                         X
  IpoProfileActivationTransaction (source exists, NOT callable)
    preview -> board qualification -> NVS commit
      -> shared Rule/lamp RAM apply -> readback -> rollback
    Gate: ClosedIpoActivationGate, always denies
                         X
   Physical outputs are never granted by IPO v2

Current real-input firmware on boot:
  Mint RAM schedule -> Rule + lamp schedule
  TP357 / SCD41 CO2 / Xiaomi -> fixed-board freshness selector
  Rule + LampSafety -> OutputSupervisor -> RF433 transport
  NVS gb_ipoprof -> read-only diagnostic stored state
  Console/e-ink -> observer (not output authority)
```

**Zapis w NVS nigdy nie jest wystarczającym dowodem aktywacji.** `stored`, `active RAM`, `requested`, `resolved`, `attempted` i `observed physical` muszą pozostawać odrębne. Samo nadanie RF433 nie potwierdza rzeczywistego przełączenia gniazda.

## Kontrakt DeviceProfile v2

- Schemat: `schemas/device-profile.v2.schema.json`; przykład `profiles/crowpanel-ipo.v2.json`; status eksportu `draft_not_applied`.
- `DeviceRegistry`: ID, driver, locator/adres, enabled oraz katalog możliwości, w tym role **przyszłe**, które nie są automatycznie sprzętowo obsługiwane.
- `SignalRegistry`: semantyczna rola, kanał, źródło podstawowe/zapasowe, priorytet i jednostki; odczyt invalid/stale/nonfinite jest nieużywalny. Dynamiczny hardware fallback nie jest obecnie dopuszczony na CrowPanel.
- `ProcessRegistry`: temperatura, RH/VPD, harmonogram lampy, ochrona CO₂; fizyczne safety nie zależy od edytowalności grafu.
- `OutputRegistry`: rola, unikatowy fizyczny endpoint, stan safe-OFF, minimalny czas ON/OFF. `ProcessBindings` ogranicza do dozwolonych efektów.
- Symboliczny `OutputArbiter` może podglądać zgodne propozycje, lecz **nie wysyła komend**.
- Limity szkicu: 24 urządzenia, 16 źródeł/wiązań sygnałów, 24 wyjścia, 16 procesów, 48 relacji, do 4 źródeł na rolę; maksimum wejściowego JSON firmware **32 KiB**, wire binary **1536 B**. Limity modelu nie stanowią deklaracji obsługi nowych urządzeń.
- Walidator B5 wymaga obecnych driverów/adresów, SCD41 CO₂, referencyjnych RF433 endpointów, pięciu istotnych linków, ograniczeń dwell i `exhaust_fan_available=true`. Jego `applyEnabled=false` nie jest błędem UI, lecz obowiązującą blokadą.

## Fizyczny CrowPanel Mint — faktycznie zaimplementowane źródła

| Funkcja | Źródło / transport | Uwaga |
| --- | --- | --- |
| Temperatura i RH wewnątrz | TP357 BLE, urządzenie 1 | Rule i niezależny LampSafety; timeout pomiaru typowo 30 s |
| CO₂ | SCD41 I²C `0x62`, urządzenie 2 | T/RH z SCD41 nie są zakwalifikowanym fallbackiem |
| Otoczenie T/RH | Xiaomi BLE, urządzenie 3 | Informacja pomocnicza |
| Zegar | DS3231 I²C `0x68` | Dzień / noc Mint |
| Wyciąg | RF433 socket 1 | Rule + safety, min ON/OFF 120/120 s |
| Lampa | RF433 socket 2 | Harmonogram + termiczne safety |
| Nawilżacz | RF433 socket 3 | Rule + safety, min ON/OFF 180/180 s |
| Wyświetlacz | SSD1680 296×128 | Obserwator, bez praw sterowania |

Mint w RAM: dzień 06:00–22:00 **24.5°C / 58% RH / 1.2 kPa**, noc **21.5°C / 65% RH / 0.9 kPa**, tryb RH, CO₂ target off. Niezależny termiczny interlock lampy: **trip ≥28°C, recovery ≤26°C przez nieprzerwane 10 minut**. Grzałki, chłodzenie, pompy, dozownik CO₂, PWM/GPIO/Modbus itp. są wyłącznie projektowymi możliwościami grafu, a nie nowymi aktywnymi fizycznymi wyjściami.

## Podział odpowiedzialności w kodzie

| Moduł | Odpowiedzialność / limit |
| --- | --- |
| `web/src/domain/ipo-*.ts` + `/` | Offline draft, walidacja, board readiness; bez live apply |
| `lib/ipo_profile/src/IpoGraph.h` / `IpoBoardAdmission.h` | Bounded model i stała referencyjna mapa CrowPanel; nie pozwolenie na RF |
| `IpoSignalSelector.h` + `RuntimeInsideSource`/`RuntimeNearbySource` | Wybór użytecznego pomiaru spośród referencyjnych źródeł |
| `IpoProfileJsonDecode.cpp` | Ścisły JSON → typowany graf i schedule; odrzucanie unsupported |
| `IpoProfileWire.h` / `IpoProfileNvsJournal.cpp` | CRC, wersje, dwa sloty w izolowanym `gb_ipoprof`; zapis nie oznacza zastosowania |
| `RuntimeScheduleProfileOwner` | Jeden bieżący profil RAM czytany przez Rule i harmonogram lampy |
| `IpoProfileActivationTransaction.h` | Planowany pojedynczy owner preview/validate/commit/apply/readback/rollback |
| `ClosedIpoActivationGate` | Produkcyjna odmowa aktywacji; nie zastępować permissive gate bez hardware sign-off |
| `OutputSupervisor` + `LampSafety` | Wyłączna własność fizycznych wyjść i nieusuwalne termiczne safety |
| `Stage28ServiceConsole` | Odrębne statusy aktywnego Mint, NVS i przygotowanej, lecz zablokowanej aktywacji |

W przypadku nieudanego rollbacku transakcja zwraca `Quarantined` i wymaga zatrzaśniętej blokady; dopóki realna supervisor-owned bramka nie istnieje, produkcyjny gate jest zamknięty. Po restarcie nie wykonujemy auto-apply z NVS.

## Interfejsy i zgodność wsteczna

- `/` — główny szkic IPO v2 (import/eksport).
- `/lab/profile-v1` — zachowany legacy v1 i jawna migracja danych.
- `/lab/v5` — badawcze v5; `/chamber-3d` — niezależna geometria.
- Climate-v6 (44 ML features/6 outputs), starsze v4 (128/15) i v5 (228/25) nie są wymienne z DeviceProfile v2.
- Historyczny `tools/panel/` obsługuje demo/research serial JSON; **nie** jest gotową, uwierzytelnioną bramką production IPO.

## Kwalifikacja i kolejność dalszych zmian

**Zaliczone:** host C++ IPO **3/3**, B5 Vitest **9/9**, host NVS fault matrix (prawdziwy plik adaptera z mock API), safety/lifecycle **3/3**, real-input CrowPanel ESP-IDF **BUILD PASS** na `1a1b3de8`. **Nie zaliczone:** test na fizycznym ESP32-S3, prawdziwy flash/NVS power cut, TP357/SCD41 urządzeniowe, output lock i API.

**Następny krok:** podłączyć `/dev/cu.usbserial-120` i fizycznie potwierdzić izolację wszystkich wyjść. Dopiero po tym wykonujemy zatwierdzone testy sprzętowe z wyjściami fake-locked; potem owner quiesce/lock OutputSupervisor, następnie bezpieczny uwierzytelniony endpoint istniejącego panelu. Szczegółowy plan: [NEXT_CHAT.md](NEXT_CHAT.md).
