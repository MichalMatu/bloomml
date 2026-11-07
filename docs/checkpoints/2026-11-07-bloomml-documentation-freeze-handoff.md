# BloomML — FINAL CHECKPOINT / dokumentacja uporządkowana / wznowienie od CrowPanel

**Data:** 2026-11-07
**Repo:** `MichalMatu/bloomml`
**Gałąź źródłowa:** `main`, kontrola Mac Local Agent: `agent-control`
**Stan:** `IPO_V2_HOST_PASS / CROWPANEL_BUILD_PASS / DEVICE_ABSENT / HARDWARE_LOCKED / NO_ACTIVE_V2`

## Decyzja handoff (wiążąca dla kolejnego czatu)

**NASTĘPNY KROK: podłączyć autoryzowany CrowPanel i potwierdzić fizyczną izolację wszystkich wyjść. Dopiero wtedy bezpiecznie rozpocząć kwalifikację sprzętu, a po niej integrację kontrolowanej bramki i uwierzytelnionego API aktywacji IPO v2.**

Ostatnie sprawdzenie Mac Local Agent: `/dev/cu.usbserial-120` **nieobecny**. Brak urządzenia ma pozostać blockerem, nie pretekstem do wykonywania ponownie zaliczonych testów lub otwierania endpointu `activate`. Nigdy nie sondować `/dev/cu.usbserial-10` ani bez zgody `/dev/cu.usbserial-1120`.

## Zweryfikowany punkt odniesienia

- W dniu dokumentacyjnego zamknięcia zastany aktualny HEAD `main`: **`c4bf62cf70b5197f5b19906cf48175a75719785e`**. Ten checkpoint i konsolidacja dokumentów są nowym commitem **docs-only**, nie zmianą wykonywalnego firmware. Na wznowieniu zawsze pobrać świeży HEAD, nie traktować powyższego jako gwarancji.
- Mac Local Agent binding `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`, repo `bloomml`, work branch `main`, `resources=[]`. Status odczytany jako `idle`, bez bieżącego zadania.
- Ostatnia przed zamknięciem kontrola samego kodu/dokumentacji: `la-bloomml-ipo-v2-hwq-docs-final-check-20261107-v1`, exit 0, clean worktree, `git diff --check` i obecność closed gate **PASS** na `c4bf62cf`.

## Co dokładnie wykonano i jaki jest dowód

| Zakres | SHA źródeł / zadanie | Wynik i ograniczenie |
| --- | --- | --- |
| IPO v2 transaction owner i closed gate | kod `977c48d`, checkpoint transakcji | Host C++ profil+aktywacja **2/2 PASS**; gate stale zamknięty |
| IPO board readiness | `977c48d` | Vitest **9/9 PASS** |
| Real-input CrowPanel N8R8 + e-ink ESP-IDF build | `1a1b3de8`; `la-bloomml-ipo-v2-crowpanel-build-20261107-v1` | **PASS**, bin `0xd1e00`, 80% app partition free; bez flash |
| Produkcyjny `IpoProfileNvsJournal.cpp` + podmieniony NVS API | `347bcca`; `la-bloomml-ipo-v2-nvs-fault-matrix-20261107-v1` | Fault matrix **1/1 PASS**, NVS jest **mock**, nie fizyczny flash |
| Niektóre krytyczne regresje safety | `347bcca` | LampSafety, TP357 stale/Rule, output lifecycle **3/3 PASS** |
| Zbiorcza bramka IPO host | `45f3527`; `la-bloomml-ipo-v2-exact-source-final-gates-20261107-v2` | `make test-ipo` **3/3 PASS** |
| Ostatnia kontrola czystości źródeł i closed gate | `c4bf62cf`; `la-bloomml-ipo-v2-hwq-docs-final-check-20261107-v1` | **PASS** |
| Fizyczny CrowPanel, prawdziwy NVS, RF/USB | Port absent | **NOT RUN / HARDWARE BLOCKED** |

W historii jest jedna nieudana próba zbiorczego `make test-ipo` z błędnym escapowaniem znaku `$` w Makefile, poprawiona komitem `45f3527`, po czym ponowiony test **PASS**. Nie wznawiać jako osobnego problemu.

**Brak dowodu:** podłączony hardware, fizyczny power-cut NVS, rzeczywiste TP357/SCD41/stale/thermal, zasoby/stack w tym firmware, flashowanie na urządzenie, supervisor quiesce, authenticated production API, aktywacja operator v2 i rzeczywiste wykonanie RF. Żaden etap host czy build nie upoważnia do twierdzenia, że profil v2 jest już aktywny.

## Nienaruszone granice bezpieczeństwa

- Rule deterministyczny pozostaje wyłączną produkcyjną polityką sterowania; ML/MPC Shadow.
- Konfigurowany fizyczny transport i plan wykonania kontroluje wyłącznie OutputSupervisor.
- TP357 LampSafety samodzielnie wyłącza lampę przy ≥28°C i dopuszcza powrót dopiero po ≤26°C przez 10 min; fizyczne safety nie może być wyłączone przez draft IPO.
- Produkcja używa `ClosedIpoActivationGate`. `IpoProfileActivationTransaction` istnieje w kodzie, ale nie ma autoryzowanego caller z przeglądarki/UART.
- Mint w RAM pozostaje aktywny przy boot, nawet gdy `gb_ipoprof` zawiera poprawne `stored`. `stored ≠ active ≠ physical ACK`. CRC32 to integralność, nie uwierzytelnianie.
- Domyślny tryb real-output w CMake to disabled; samo `0` we flagach **nie zastępuje fizycznej izolacji wyjść**.

## Oczyszczenie aktywnej dokumentacji

- `docs/CURRENT_STATUS.md`: pojedyncza prawda `PASS/BLOCKED`, bez powielania dawnych Bridge/B5 i nieaktualnego wznowienia benchmarków Mac/GPU.
- `docs/CONFIGURATION_FLOW.md`: wyłącznie bieżący kontrakt grafu/runtime, przepływy ownerów oraz source truth; bez historycznych „UNVERIFIED / nie uruchamiano testów”.
- `docs/PROJECT_ROADMAP.md`: jedno P0→H0→H1→H2→H3, jawne blokery, osobne przyszłe ML/UI.
- `docs/NEXT_CHAT.md`: gotowa instrukcja startowa od portu i izolacji z rozróżnieniem bramek.
- `docs/ARCHITECTURE.md`, `docs/PRODUCT_DIRECTION.md`, `docs/CHANGELOG.md`, `docs/HISTORY.md` i `docs/README.md`: spójne granice i nawigacja. Starsze materiały i poprzednie checkpointy zachowane jako historia/dowody, **nie usunięto źródeł ani testów**.
- Nie czyszczono `agent-control` przez kasowanie dowodów agentów; kontrola tej gałęzi jest rejestrem wykonania.

## Warunki dalszej pracy (gates)

1. **H0 physical preflight:** CrowPanel widoczny na `/dev/cu.usbserial-120`, fizyczne wyjścia odłączone/izolowane, tożsamość sprzętu i zgoda na bezpieczne działania potwierdzone. Gdy brak: `BLOCKED` i stop.
2. **H1 device acceptance:** exact-SHA fake-locked build/flash wyłącznie po autoryzacji; real NVS/reboot/power-loss, TP357/SCD41 invalid/stale i niezależny LampSafety/Rule, resource metrics; dowód bez emisji RF.
3. **H2 runtime interlock:** jedna supervisor-owned quiescence + emergency lock, serializacja względem 1 s control loop; test rollback i quarantined; niezależna akceptacja bezpieczeństwa.
4. **H3 local API:** uwierzytelnione i autoryzowane preview/activate/status/rollback oraz połączenie z istniejącym konfigurator-em. Brak drugiego engine wyjść i auto-apply z NVS.

## Dokumenty do otwarcia po powrocie

1. `AGENTS.md` — repo, uprawnienia, zabronione porty i work mode.
2. [NEXT_CHAT.md](../NEXT_CHAT.md) — jeden następny krok i bieżąca sekwencja.
3. [CURRENT_STATUS.md](../CURRENT_STATUS.md) — prawdziwy stan.
4. [CONFIGURATION_FLOW.md](../CONFIGURATION_FLOW.md) i [ARCHITECTURE.md](../ARCHITECTURE.md) — właściciele, graf i bramki.
5. [PROJECT_ROADMAP.md](../PROJECT_ROADMAP.md) — priorytety.
6. [Hardware build/NVS checkpoint](2026-11-07-bloomml-ipo-v2-hardware-qualification-gate.md) oraz [Transaction checkpoint](2026-11-07-bloomml-ipo-v2-activation-transaction.md) — szczegółowe dowody.

**Zakaz trybów:** Codex, ChatGPT Work, GitHub Actions, nowe gałęzie, force-push, niedozwolone USB i nieautoryzowane fizyczne wyjścia. Wszelka fizyczna kwalifikacja musi być na dokładnym SHA faktycznie testowanego firmware.
