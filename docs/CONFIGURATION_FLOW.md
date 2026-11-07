# BloomML — jeden przepływ konfiguracji i sterowania (kanoniczna mapa)

**Stan: 2026-11-07 · profil v1 = walidowany szkic, NIE zastosowany na ESP32-S3.** To jest jedno miejsce do rozumienia parametrów, czujników, wyjść, ograniczeń i kolejnych bramek. Nie jest to kolejny panel. Źródła zachowania fizycznego mają pierwszeństwo przed opisem: `src/climate/runtime/RuntimeAdapters.cpp`, `runtime/schedule/ScheduleProfile.cpp`, `output/OutputBindings.cpp`, `lib/environment_control/src/climate/ClimatePolicy.cpp`.

## Co ma zobaczyć operator

```text
/ — Jedna konfiguracja growboxa
[1] Wybieram faktycznie zainstalowane czujniki i wyjścia
    ↓
[2] Wybieram cel T + RH XOR VPD, pory dnia/nocy
    ↓
[3] Przegląd: co jest możliwe, co tylko obserwowane, czego brakuje
    ↓
[4] Pobieram/wczytuję walidowany profil device-profile.v1 (DRAFT — działa)
    ↓
[5] [PLANOWANE] Firmware: import + sprawdzenie + NVS + atomowe apply/readback
    ↓
[6] [PLANOWANE] Realtime status w panelu; aktualny e-ink już jest obserwatorem
    ↓
ESP32-S3: czujniki → Rule + pasywne ML shadow → safety →
OutputSupervisor → 3 realne kanały RF433 → telemetria
    ↓
prawdziwe trajektorie → kalibracja fizyki → offline MPC/trening → nowe shadow
```

Dzisiaj punkty **1–4** są nową stroną *szkicu*, a firmware używa nadal stałego profilu Mint. Punkty **5–6 nie są zaimplementowane**: UI nie ma łączności ze sprzętem ani przycisku pozorującego zapis. Widok `/lab/v5` zachowuje stary eksperymentalny, obszerny konfigurator 228 cech; `/chamber-3d` to osobny *geometryczny* podgląd bez synchronizacji stanu (nie symulator klimatu).

## Prawdziwe źródła sygnałów — obecny CrowPanel

| Czujnik / źródło | Mierzy / dostarcza | Używane przez firmware | Czy można teraz wybrać dowolny zamiennik? |
| --- | --- | --- | --- |
| **TP357 BLE (wewnątrz)** | temperatura powietrza °C; RH % | **obowiązkowe** T/RH dla bieżącego Rule, VPD, alarmu lampy; age/valid | **Nie.** To bieżące źródło temperatury i wilgotności. |
| **SCD41 I²C 0x62** | CO₂ ppm (czujnik ma również T/RH) | obecnie **tylko CO₂**. T/RH z SCD41 nie są podłączonym fallbackiem | **Nie.** CO₂ opcjonalny pomiar. |
| **Xiaomi BLE (poza komorą)** | temperatura i RH otoczenia | kontekst wymiany przez wyciąg; bez świeżych danych reguły wentylacji mają mniej informacji | **Nie.** Inne modele wymagają adaptera. |
| **DS3231 I²C 0x68 / zegar** | czas | profil dzienny/nocny, harmonogram lampy | **Nie.** Zegar musi być wiarygodny. |
| **E-ink SSD1680** | status / diagnostyka | wyłącznie odbiorca danych, nie czujnik i nie właściciel wyjść | Nie dotyczy. |

Pomiar ma **value, valid, age_ms**. Brak danych, stary pomiar i fałszywe zero są różnymi stanami. Domyślny timeout Rule to **30 000 ms**. Obecne źródła BLE są identyfikowane w konfiguracji builda, nie poprzez dynamiczne skanowanie i przypisywanie roli w GUI. Nie zakładać, że samo zaznaczenie `installed` oznacza fizyczne wykrycie.

## Wyjścia fizyczne vs role modelu

| Faktycznie powiązane wyjście | Role / endpoint | Na co może oddziaływać | Sposób decyzji / ograniczenie |
| --- | --- | --- | --- |
| **Wyciąg** | `exhaust_fan` → `rf433_socket_1` | T, RH/VPD, pośrednio CO₂ | Rule uruchamia, gdy wymiana z otoczeniem poprawia klimat; termiczne / wysokie CO₂ safety może wymusić wentylację; binary dwell |
| **Lampa** | `scheduled_light` → `rf433_socket_2` | światło i ciepło (skutek uboczny) | TYLKO harmonogram + niezależny interlock temperatury; **nie** ma regulacji PPFD/DLI ani jasności przez ML |
| **Nawilżacz** | `humidifier` → `rf433_socket_3` | RH ↑, VPD ↓ | Rule i binary dwell, z możliwością blokady safety |

Sześć ról modelu climate-v6 to: grzałka, chłodzenie, wyciąg, nawilżacz, osuszacz, dozownik CO₂. Produkcyjny output supervisor mapuje obecnie **tylko wyciąg i nawilżacz** z tych sześciu; lampa jest siódmym, osobnym wyjściem harmonogramowym. Grzałka, chłodzenie, osuszacz, dozownik CO₂ ani wentylator obiegowy nie są obecnie fizycznie podłączone w tym profilu. Ich obecność w schema/symulatorze to **nie** dostępna funkcja operatora.

**Sprzęt RF433 jest sterowaniem binarnym ON/OFF**, nie płynną regulacją 0..1; wartości z polityki są arbitrowane do progów/dwell. Dla wentylatora minimum ON/OFF = 120/120 s, dla nawilżacza 180/180 s według domyślnych zasad. `OutputSupervisor` i prawda o transport/ACK pozostają właścicielem finalnego wykonania.

## Parametry i zależności: co naprawdę można regulować

| Cel operatora | Jakie wejście jest konieczne | Co realnie może zrobić obecny kontroler | Co pozostaje niemożliwe / warunkowe |
| --- | --- | --- | --- |
| Temperatura °C | świeże TP357 T + RH | uruchomić wyciąg, jeżeli otoczenie poprawi T; termiczne wyłączenie lampy | bez grzałki i chłodzenia nie gwarantuje osiągnięcia nastawy ani ocieplenia |
| Wilgotność RH % | TP357 T + RH | nawilżanie; wyciąg przy korzystniejszej absolutnej wilgotności otoczenia | bez osuszacza nie gwarantuje obniżenia RH |
| **VPD kPa** zamiast RH | TP357 T + RH, VPD liczony, nie miernik VPD | te same dostępne wyjścia, inna metryka błędu | RH i VPD to **alternatywne** tryby sterowania, nie dwa niezależne cele |
| CO₂ ppm | SCD41 świeży/valid | pomiar; wysokie CO₂ mogą aktywować wymuszenie bezpieczeństwa wyciągu | **brak dozownika**; nie można dziś podać docelowego CO₂ jako aktywnego setpointu |
| Dzień/noc | DS3231, czas lokalny Europe/Warsaw | przełącza nastawy i włącza/wyłącza lampę | bez RTC harmonogram nie jest wiarygodny; aktualna logika reaguje na pełne godziny |
| Światło (PPFD/DLI) | brak produkcyjnego czujnika światła | lampa ON/OFF wg harmonogramu | **brak kontroli rzeczywistej PPFD/DLI**, nawet jeżeli symulator ma te pola |
| Doniczki, gleba, podlewanie | brak podłączonego runtime | tylko v4/v5 research i geometria | brak obecnych fizycznych pętli sterowania |

**Parametry draft v1:** `profile_id`; deklaracje czterech obecnych źródeł i trzech fizycznych kanałów; `humidity_mode` RH/VPD; `targets.day/night.air_temperature_c` (10–40°C), `relative_humidity_pct` (20–90%), `air_vpd_kpa` (0.2–3.5 kPa); `schedule.day_start_hour`, `night_start_hour` (0–23, różne); `sensor_timeout_ms` (5000–300000 ms). Zakresy są dla walidacji profilu, nie limitami hard safety. **Draft nie zmienia parametrów runtime.**

**Obecny Mint w firmware (sztywno w `ScheduleProfile.cpp`):** dzień 06:00–22:00, 24.5°C / 58% RH / 1.2 kPa; noc 21.5°C / 65% RH / 0.9 kPa; tryb RH; CO₂ target disabled; timeout 30 s; wyciąg i nawilżacz enabled; lampa dzień ON/noc OFF. Progi bezpieczeństwa lampy: wyłączenie przy **≥28°C**, dopuszczenie po **≤26°C przez ciągłe 10 min**. Te progi nie są edytowalne w profilu i nie mogą być nadpisywane przez ML. W `ClimatePolicy.cpp` znajdują się osobne progi high/low T, high RH i high CO₂; nie mieszać ich z docelowymi nastawami.

## Jeden kontrakt, bez mieszania schematów

- **Nowy dokument operatora:** `schemas/device-profile.v1.schema.json` — JSON Schema 2020-12. Przykład: `profiles/crowpanel-mint.v1.json`. `deployment_status=draft_not_applied` jest wymagany i nie oznacza NVS ani autoryzacji wyjść.
- **Strona operatora:** `web/src/pages/setup-page.tsx` + `web/src/domain/device-profile.ts`: formularz, import, walidacja, eksport; nie ma backendu urządzenia.
- **ML inference v6:** `schemas/environment-controller.v6.json` (44 wejścia/6 decyzji) — inna rzecz niż profil operatora; firmware ma swoje aktualne typy `ClimateTypes.h`.
- **Research v5:** `web/schema/environment-controller.v5.json` (228/25), dostępny w `/lab/v5`; **nie** jest migratorem do v1.
- **Research v4:** `schemas/environment-controller.json` (128/15); nie mapować 1:1 na klimatyczne wyjścia ESP.
- **Symulator termodynamiczny R4 + MPC:** `tools/ml/physics/` i `tools/ml/mpc/` są offline; wyniki syntetyczne bez kalibracji nie włączają automatycznej kontroli urządzeń.

## Dokładny następny krok integracyjny

1. Dodać wersjonowany, **typowany** odczyt profilu firmware z walidacją przed apply, z obsługą niedostępnych czujników i ról. **Nie zmieniać** istniejącej fizycznej mapy RF433 tylko dlatego, że JSON ma pole endpoint; w v1 obowiązują stałe mapowania. Profil nie może ustanawiać nowego output ownera.
2. Zastąpić wyłącznie hard-coded nastawy `ScheduleProfile.cpp` wartościami z już zwalidowanego snapshotu w runtime, w taki sposób, aby harmonogram lampy i cele klimatu korzystały z tego **samego snapshotu**. Fail-safe przy błędzie RTC lub profilu; atomowe apply, bez częściowej aktualizacji.
3. Zapisywać profil do odrębnej domeny NVS z wersją, CRC/checksum, rollbackiem i przywróceniem Mint; istniejący output state/policy persistence pozostaje osobną odpowiedzialnością.
4. Dodać jednolity *readback* urządzenia: rzeczywisty aktywny hash/wersja profilu, odczyty/age/valid, aktywny cel, decyzję Rule, propozycję shadow, zastosowany stan i status transportu. Widok e-ink ma być tylko obserwatorem.
5. Dopiero wtedy wyposażyć stronę operatora w transport **preview → apply → readback**, rozdzielając wysłanie od faktycznego potwierdzenia. Nie używać w tym celu `tools/panel` v4/serial bez bezpiecznego, nowego adaptera.
6. Po integracji, na osobne zlecenie: celowe testy migracji/safety/rollback, rzeczywiste czujniki i hardware. **Nie powtarzać wcześniejszych ciężkich kompilacji i testów bez zmiany wymagającej tej bramki.**

## Co nie jest definicją sukcesu

Eksport szkicu ≠ zapis profilu na ESP. Podłączony czujnik w formularzu ≠ świeży pomiar. Prośba sterownika ≠ rzeczywiście wykonany przełącznik. Model wytrenowany na fizyce R4 ≠ poprawnie skalibrowany growbox. Shadow może wyjaśnić, co *zaproponowałby*, ale aktualnie nie ma prawa sterować RF433.
