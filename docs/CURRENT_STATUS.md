# BloomML — aktualny stan

**2026-11-07 · źródło: `MichalMatu/bloomml` / `main`**

**STATUS: IPO v2 HOST PASS / CROWPANEL BUILD PASS / DEVICE NOT QUALIFIED / ACTIVATION CLOSED.**

**JEDYNY NASTĘPNY KROK:** podłączyć autoryzowany CrowPanel do Maca i potwierdzić **fizyczną izolację wszystkich wyjść**. Ostatni odczyt Mac Local Agent: `/dev/cu.usbserial-120` **ABSENT**. Nie otwierano portów, nie flashowano ani nie uruchamiano RF. Dopóki obecność i izolacja nie zostaną zweryfikowane, nie rozpoczynać testów sprzętowych ani nie udostępniać API aktywacji. [Instrukcja wznowienia](NEXT_CHAT.md).

## Co naprawdę działa w źródłach

| Warstwa | Stan i granica |
| --- | --- |
| Sterowanie | Deterministyczny **Rule**; ML/MPC wyłącznie Shadow/research |
| Wyjścia | Jedyny normalny właściciel: **OutputSupervisor**; RF433 1 wyciąg, 2 lampa, 3 nawilżacz |
| Bezpieczeństwo | Niezależny TP357 LampSafety: trip ≥28°C; recovery ≤26°C nieprzerwanie przez 10 minut |
| Operator | `/` to offline szkic DeviceProfile v2; walidacja grafu, board readiness, import/eksport; **brak apply do sprzętu** |
| Wejścia | CrowPanel reference selector: TP357 T/RH, SCD41 CO₂, Xiaomi ambient T/RH; filtr valid/finite/age; brak dynamicznej rejestracji driverów |
| Harmonogram | Jeden `RuntimeScheduleProfileOwner` w RAM dla Rule i lampy; na starcie **wbudowany Mint** |
| NVS IPO | Dwa sloty `gb_ipoprof`, CRC, generation, rollback; boot tylko **odczytuje stan** do diagnostyki |
| Transakcja | `IpoProfileActivationTransaction`: preview → validate → NVS commit → RAM apply → readback/rollback. Produkcyjny `ClosedIpoActivationGate` **zawsze odmawia**; brak wywołania z UI/UART |
| Odczyt statusu | Rozdzielone `ipo_store` / `ipo_profile` / `ipo_readback` / `ipo_activation`. Zapisany NVS ≠ aktywny Mint ≠ fizyczny stan RF |
| E-ink | Native SSD1680 + Clay, jeden framebuffer, asynchroniczny obserwator; przyciski wymagają odrębnej kwalifikacji fizycznej |

Źródło konfiguracji i granice danych: [CONFIGURATION_FLOW.md](CONFIGURATION_FLOW.md); architektura: [ARCHITECTURE.md](ARCHITECTURE.md).

## Zaliczone bramki — nie powtarzać bez zmiany dotyczącego ich kodu

| Test | Wykonanie | Wynik |
| --- | --- | --- |
| Real-input CrowPanel N8R8 + e-ink ESP-IDF build | Mac Local Agent, kod `1a1b3de8` | **PASS**, bin `0xd1e00`; 80% partycji aplikacji wolne |
| Host IPO parser/activation/NVS journal | `make test-ipo`, kod `45f3527` | **3/3 PASS** |
| Produkcyjny `IpoProfileNvsJournal.cpp` z mock NVS fault injection | kod `347bcca` | **1/1 PASS** (zawarte w 3/3) |
| LampSafety, BLE stale → Rule, output lifecycle | kod `347bcca` | **3/3 PASS** |
| IPO board readiness Vitest | wcześniejszy kod `977c48d` | **9/9 PASS** |
| Końcowy przegląd dokumentów i closed gate poprzedniego etapu | kod `c4bf62cf` | **PASS**, czyste repo |

*Ważne:* NVS fault injection używa podmienionego API, **nie fizycznego flash**. Kompilacja ESP-IDF nie dowodzi zachowania czujników ani poprawnego działania na podłączonej płytce. SHA testów i builda są różne; późniejsze zmiany dotyczyły testów i dokumentacji, nie produkcyjnej ścieżki firmware.

## Otwarte, zależne od obecności sprzętu

1. Potwierdzić port `/dev/cu.usbserial-120`, tożsamość CrowPanel i faktyczną izolację RF/wyjść. Nie dotykać `/dev/cu.usbserial-10`; `/dev/cu.usbserial-1120` wyłącznie za osobną zgodą.
2. **Dopiero wtedy** wykonać zatwierdzone, izolowane testy realnego ESP-IDF NVS, odczytów TP357/SCD41 (brak/invalid/stale), termiki Rule/LampSafety, reset/recovery i pomiarów heap/PSRAM/stack/watchdog. Wyjścia mają pozostać fake-locked.
3. Po pozytywnym hardware gate: zaprojektować i zweryfikować jedną bramkę quiesce/emergency-lock należącą do OutputSupervisor, z synchronizacją cyklu i autoryzacją operatora.
4. Dopiero po tych zabezpieczeniach: uwierzytelniony kanał preview/activate/status/rollback i połączenie z **istniejącym** panelem IPO. Brak automatycznego apply po restarcie oraz brak pozornego potwierdzenia RF.

**Poza tym sprintem:** badawczy MPC ma historycznie **19/22** niezaliczonych trudnych scenariuszy; nie otrzymuje uprawnień do wyjść. Tematy Clay, ML/TimesFM, optymalizacji Maca i rozbudowy driverów nie są aktualnym punktem wznowienia. Historia: [HISTORY.md](HISTORY.md), [CHANGELOG.md](CHANGELOG.md), [research status](simulator/RESEARCH_STATUS.md).

**Dowody:** [finalny checkpoint](checkpoints/2026-11-07-bloomml-documentation-freeze-handoff.md) · [build + NVS matrix](checkpoints/2026-11-07-bloomml-ipo-v2-hardware-qualification-gate.md) · [transakcja IPO](checkpoints/2026-11-07-bloomml-ipo-v2-activation-transaction.md).
