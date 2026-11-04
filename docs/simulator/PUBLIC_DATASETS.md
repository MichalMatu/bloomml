# BloomML — publiczne dane klimatu szklarni: audyt i wybór (2026-11-04)

**Stan:** discovery i ograniczony pilotaż danych **zakończony**. To dane *innych szklarni*, nie pomiary naszego growboxa. Użycie do **zewnętrznego benchmarku i badań transferu**: tak; do deklaracji fizycznej kalibracji naszego urządzenia lub treningu autonomicznego sterowania: **nie**. Bieżący status: [RESEARCH_STATUS.md](RESEARCH_STATUS.md).

## Wybrane źródła — priorytet, nie mieszaj domen i typów danych

| Priorytet | Zbiór / źródło | Prawdziwe zmienne i zastosowanie | Licencja (źródło) | Stan dowodu i ograniczenia |
| --- | --- | --- | --- | --- |
| **A1 — start operacyjny** | [NUTRICONTROL, Hiszpania](https://zenodo.org/records/21037154), [GitHub](https://github.com/SmartGreenHouses/NUTRICONTROL_datasets) | Lata 2021–2026, 3 lokalizacje / 6 komór: T/RH, pogoda, okna, wentylacja, grzanie, VPD (zależnie od lokalizacji). **Forecast T/RH, porównania między obiektami i baseline** | **CC BY 4.0** (Zenodo i README; podaj autorów/DOI i zmiany) | **Zweryfikowano fizyczne pliki** 3 komór, SHA256, spójny czas, poprawny importer i baseline. Wiele braków w RH/stanie urządzeń. **Brak CO₂ i bezpośredniego PPFD; czas bez potwierdzonej strefy.** |
| **A2 — CO₂/PAR, przebadany mirror** | [Wageningen AGC 2019, DOI 4TU](https://doi.org/10.4121/uuid:88d22c60-21b3-4ea8-90db-20249a5be2a7), [kopia Kaggle](https://www.kaggle.com/datasets/piantic/autonomous-greenhouse-challengeagc-2nd-2019) | 6 szklarni × 47 809 = **286 854** odczytów co 5 min, CO₂/T/RH/PAR, pogoda i stany sterowania; próbny transfer między szklarniami | **CC0** wg 4TU i etykiety Kaggle | **16/16 testów i benchmark PASS** dla ZIP mirror przypiętego SHA256; niemal 99,85% kompletności. **Oryginalny 7z 4TU nadal niepobrany (API timeout), równoważność mirror ↔ oryginał niezweryfikowana.** [Wyniki i negatywny test transferu](WUR_DATASET_EVALUATION.md). |
| **A3 — niezależna walidacja fizyki** | [Korea, paprika / 278 dni](https://data.mendeley.com/datasets/pf24hxwygf/1) | 40 032 rekordy co 10 min; **oddzielne zmierzone i symulowane** T/RH oraz stany/pozycje okien, ogrzewanie, ekrany; test warunków i błędów symulatora | **CC BY 4.0**, Mendeley | Opis pierwotny sprawdzony, pliki **jeszcze niepobrane**. **Nigdy nie używać kolumn `simulated` jako rzeczywistych etykiet!** Log-gap flag i konwencje czasu są opisane w README. |
| **B1 — krótszy, wysoka częstość CO₂** | [BYU GETS 2025](https://scholarsarchive.byu.edu/data/101/) | 4 tygodnie, co 1 min, CO₂ ppm, RH %, temperatura **gleby** w °F; detekcja anomalii, CO₂/RH baseline | **CC BY 4.0**, BYU | Niesprawdzone rekordy pliku; brak gwarantowanego T powietrza i stanów aktorów; lokalny czas bez zweryfikowanej strefy; przeliczyć °F na °C tylko dla *soil_temp*. |
| **B2 — sygnały grzania i okien** | [Shirvan, Iran 2023–2024](https://data.mendeley.com/datasets/ncrm9p4tc8/1) | 6 miesięcy, wewn./zewn. T/RH, wiatr, **lux**, heater/window status. Forecast i korelacje działań | **CC BY 4.0**, Mendeley | Pliki niesprawdzone. **Lux ≠ PPFD** bez kalibracji widmowej. Brak CO₂. |
| **B3 — światło i rośliny** | [Belgia: mikroklimat pomidorów 2018–2020](https://data.mendeley.com/datasets/tkbkzdt5nr/2) | T/RH z SHT31 i systemu Priva, **PAR**, pomiary roślin; walidacja mikroklimatu i proxy oświetlenia | **CC BY 4.0**, Mendeley | Pliki jeszcze niesprawdzone; brak potwierdzonej ciągłej telemetrii sterowania |
| **B4 — gradienty / R5** | [Algieria: 5 dni 2019](https://data.mendeley.com/datasets/3dw54yhhcc/1) | 1 min, dwie wysokości T/RH, zewnętrzna T/RH, promieniowanie, wiatr; sprawdzanie **założenia 0D vs pionowe gradienty** | **CC BY 4.0**, Mendeley | Tylko 5 dni / 7200 próbek; niesprawdzone pliki, brak CO₂ i sterowania |
| **EXCLUDE** | [Pomona / HuggingFace](https://huggingface.co/datasets/Okyanus/greenhouse-sensor-data) | Mieszanka danych, przetworzonych przykładów JSONL, syntetyki i odpowiedzi instrukcyjnych | karta repo: CC BY 4.0 | **Nie używać do walidacji fizyki** ani jako ground truth. Potencjalny leakage, brak wiarygodnego rozdzielenia sygnału realnego od pochodnego |

**Ranking:** NUTRICONTROL — zweryfikowane 3 pliki i T/RH benchmark; **WUR 2019 — 6 komór, CO₂/PAR benchmark i negatywny wynik prostego modelu transferowego**, ale mirror nie jest ostatecznie porównany z oryginałem. Korea — następne źródło do walidacji różnicy `measured` vs `simulated`. Żadne z tych danych nie są kalibracją naszego growboxa ani zatwierdzonym źródłem do modelu sterującego.

## Pilotaż na realnych, przypiętych plikach NUTRICONTROL

Publiczny GitHub commit źródła: `6dece0d2c7483982610e4d2de225db3c4649b935`; **oficjalny tag `V1.0.0` ma te same Git blob SHA dla wszystkich trzech użytych CSV**, co potwierdzono niezależnie przez GitHub API (tag commit `f875925e7c1933f978fde15775a8988c26fcbe23`). Przetworzona seria ma rozdzielczość **15 minut**; brak imputacji deklarowany przez autorów. Trzy pliki pobrane i ocenione przez Mac Local Agent; po pomiarze pliki surowe **usunięto z katalogu tymczasowego**. Utrwalone tylko sumy SHA256, liczności, pokrycie i metryki. Álava ma dodatkowo **169 przerw 30-minutowych i 5 przerw 45-minutowych** w szeregu dat; pozostałe dwie komory mają równy interwał znaczników 15 minut (nie oznacza to kompletności wartości). Wyszukiwanie czasu nie pokazało duplikatów ani cofania w obrębie żadnego pliku, ale format `dateTime` jest **naive**, nie przypisywać automatycznie UTC ani Europe/Madrid.

| Komora | Wiersze | T kompletne | RH kompletne | Aktory/ustawienia | Ocena |
| --- | ---: | ---: | ---: | --- | --- |
| Álava | **80 548** | **95,1%** | **82,2%** | forced vent 18 281 / 80 548; okna ~22–23 tys. | **Wybrana do forecast T i wstępnego RH** |
| Cartagena C0 | **69 792** | **95,9%** | **39,6%** | wentylacja/okna obserwowane tylko fragmentami | **T do benchmarku; RH do wstrzymania** |
| Navarra C0 | **168 668** | **93,8%** | **74,2%** | heater 16 841 / 168 668; okno 60 060 | **T do benchmarku; RH najpierw audyt braków** |

### Pierwszy rzeczywisty benchmark — *persistence*, nie wynik naszego modelu

Metoda: chronologiczny holdout **ostatnich 20% różnych dni** per komora; prognoza `y(t+h)=y(t)`, bez treningu na dniach testowych, bez imputacji i bez łączenia szklarni. Zliczane wyłącznie pary z dokładnym odstępem 15 lub 60 minut, z dwoma rzeczywistymi odczytami. Dla `1 i 5 min` wynik **unsupported**, nie interpolować. Miary: MAE/RMSE; tutaj MAE. RH w **punktach procentowych**, T w °C.

| Komora | T MAE 15 min | T MAE 60 min | RH MAE 15 min | RH MAE 60 min |
| --- | ---: | ---: | ---: | ---: |
| Álava | **0,384°C** (n=15 143) | **1,169°C** (n=15 055) | **1,161 pp** (n=10 562) | **3,619 pp** (n=10 873) |
| Cartagena C0 | **0,340°C** (n=13 094) | **1,206°C** (n=13 031) | 1,215 pp (n=4 327, niepełny) | 3,718 pp (n=4 146) |
| Navarra C0 | **0,438°C** (n=31 238) | **1,258°C** (n=31 067) | 1,895 pp (n=22 521) | 4,979 pp (n=22 348) |

**Dlaczego to ważne:** następnym razem fizyczny model, LSTM lub TinyML student musi pokonać ten baseline na **tych samych datach, sygnałach i horyzontach**, a nie na przypadkowym train/test split. Dla tego pilotażu nie uczono żadnego modelu. Te liczby nie mierzą jakości istniejącego symulatora BloomML. Braki RH i okien mogą być skorelowane z przebiegiem urządzeń — wymaga osobnej analizy mechanizmu braków, nie losowego uzupełniania.

### SHA256 (pinned 2026-11-04)

- `Alava_data/clean_data/Alava_15min.csv`: `20873873821550b73b9afe95a364c9ac8c572d325143befcd0885650212e3fbd`
- `Cartagena_data/clean_data/Cartagena_comp0_15min.csv`: `b68e5b84d79085236719fb9d9536a0ae5b48ce0ffd15ed3d1cb978bcd248c29c`
- `Navarra_data/clean_data/Navarra_comp0_15min.csv`: `6ea4aa27dbf1786ca791688b5d9090aa363fcc5f77b795844931d6e8188a85a8`

Mac Local Agent raw schema/audit: `agent-control:.agent/results/la-bloomml-public-greenhouse-dataset-audit-20261104-v1.json`, a pinned source + benchmark: `agent-control:.agent/results/la-bloomml-public-datasets-baseline-20261104-v1.json`; oba `done`, exit 0. Kod w repozytorium: `tools/ml/external_greenhouse_benchmark.py`, testy: `tests/test_external_greenhouse_benchmark.py` (**7/7 PASS na Macu**, bez instalacji pakietów).

## Kontrakt przyjęcia do treningu i walidacji

**Dopuszczalny do badań:** jawna licencja i oryginalne źródło, fizyczny pomiar oznaczony `measured_external_processed_not_own_growbox`; SHA256 pliku, jednostka SI, źródłowy zegar/strefa i jakość. Dopuszczony baseline T/RH wyłącznie w komorach i horyzontach, w których jest wystarczające pokrycie i pewny sens targetu.

**Wymagane przed nauką prognozy:** potwierdzić czas/strefę źródła, kalibrację sygnałów i procedurę QC, podział na odrębne dni/komory, brak leakage z kolumn sim/realized setpoint/future weather, raportować persistence. Nie wprowadzać danych zdarzeń urządzeń jako zmierzonego przepływu lub rzeczywistego poboru W.

**Wymagane przed fizyczną walidacją BloomML:** znana geometria i sensor positions, rzeczywiste strumienie powietrza, zużycie energii/światło, stany urządzeń i parametry chłodzenia/wentylacji; dane szklarni przełożone na inne warunki to *transfer/out-of-domain*, nie nasz growbox.

**Wstrzymane:** trenowanie końcowego modelu sterującego na ESP32, bez realnych logów własnego urządzenia, testu niezależnych dni, sprawdzania bezpieczeństwa i referencyjnego Rule. Brak fizycznego autorytetu ML.

## Najbliższe zadania

1. **P1:** pobrać i rozpakować **oryginalny Wageningen CC0 v2**, sprawdzić faktyczne CO₂/PAR/urządzenia, strefy czasowe, braki i dni, zabezpieczyć split po komorze; nie używać pochodnych CSV bez niezależnej kontroli.
2. **P1:** zbadać, czy okna, grzanie, wyciąg mają *zmierzoną aktywność* czy tylko setpoint w czasie; nie łączyć kontrolera z fizycznym efektem bez identyfikacji.
3. **P2:** pobrać Korea 2026; jawne `measured` vs `simulated`, holdout i porównanie prawdziwego solvera BloomML do par zmierzonych przy tych samych wymuszeniach.
4. **P2:** ustandaryzować importer na wiele źródeł dopiero po rzeczywistych 2–3 mapowaniach; bez sztucznego uniwersalnego loadera.
5. Dane źródłowe pozostają poza repozytorium BloomML, pobierane jako jawnie licencjonowane snapshoty; Kaggle tylko jako offline compute. `main`, firmware, Growclip bez zmian.
