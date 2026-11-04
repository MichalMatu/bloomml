# Wageningen AGC 2019 — rzeczywiste dane i próbny transfer między szklarniami

**Status: 2026-11-04, EXPLORATORY / źródło oryginalne nadal do porównania.** Pierwsza użyteczna ocena CO₂, T, RH i PAR z zewnętrznych szklarni. Nie jest to walidacja fizyki BloomML ani podstawy do sterowania ESP32.

## Źródło i ograniczenia licencji / wiarygodności

- **Oryginał:** Silke Hemming i in., *Autonomous Greenhouse Challenge, Second Edition (2019)*, 4TU.ResearchData, wydanie 2 (2020), [DOI 10.4121/uuid:88d22c60-21b3-4ea8-90db-20249a5be2a7](https://doi.org/10.4121/uuid:88d22c60-21b3-4ea8-90db-20249a5be2a7), licencja **CC0**. Źródłowy plik `AutonomousGreenhouseChallenge_edition2.7z`, 8 418 715 B, MD5 `2a0c7f3332881caef54ca8f4dc60c9a3`.
- Dwie próby dostępu do **oficjalnego API 4TU** z MacBooka `/v2/articles/<uuid>` i `/files`: **timeout**. Oryginalnych bajtów 7z nie pobrano. **Nie wolno twierdzić, że mirror jest identyczny z oryginałem.**
- **Dostępna kopia:** [Kaggle: piantic/autonomous-greenhouse-challengeagc-2nd-2019](https://www.kaggle.com/datasets/piantic/autonomous-greenhouse-challengeagc-2nd-2019), etykieta licencji Kaggle **CC0-1.0**, pobrane ZIP 12 793 984 B, SHA-256 **`c19d385e7f4731a77e59ebfd763fd49e35e7ac4128d0aaf41b5717709cd75a68`**. 45 plików, sześć `<nazwa>/GreenhouseClimate.csv`, `Weather/Weather.csv` i materiały pomocnicze. Źródła danych pozostają *zewnętrzne i wtórnie opublikowane*.
- **Godzina:** `%time` w serialu Excela z dokładnością do pięciominutowej siatki. Daty 2019-12-16 do 2020-05-30, **strefa czasowa niezweryfikowana**, więc nie przypisujemy UTC. CSV nie zawierają jawnych datetime/UTC.
- **Znaczenie:** `Tair` temperatura °C, `Rhair` RH %, `CO2air` ppm; `Tot_PAR` raportowany sygnał promieniowania PAR, lecz kanał mierzony vs pochodny i metadane sensora wymagają jeszcze potwierdzenia w oryginalnym opisie. `*_sp` nastawy, `*_vip` realizowane nastawy sterowania; **nie** utożsamiać ich z fizycznym przepływem powietrza czy mocą.

## Audyt rzeczywistych plików kopii Kaggle

- **6× 47 809 = 286 854 rekordy klimatu**, dodatkowo 47 809 rekordów pogody; 50 kolumn na komorę i 11 w pogodzie, każda seria `%time` monotoniczna bez duplikatów, odstęp znaczników pięć minut bez luk. Nie oznacza to pełnej kompletności *wartości*.
- Wskaźniki T, RH, CO₂ i PAR kompletne w około **99,85%**. Zwykle po 71 pustych/NaN wartości w końcu serii; dla `Reference` dodatkowe niepoprawne wartości.
- **Reference — wymagany przegląd przed treningiem:** 2 pomiary temperatury poniżej 5°C (minimum −1°C), 1 pomiar RH poniżej 10% (minimum 7,7%); 2 niepoprawne wartości CO₂ i 3 RH odrzucone *jako cel numeryczny* według fizycznych ograniczeń parsera. Skrajnych **prawidłowych składniowo** wartości nie wycinano — oznaczono je ostrzeżeniem. Pozostałe pięć komór: zero flag skrajnych wg zdefiniowanych progów w tym audycie.
- Przykładowe zakresy CO₂: AICU 379–2000 ppm, Digilog 257–1272 ppm, TheAutomators 396–1560 ppm. Nie utożsamiać wszystkich wysokich CO₂ z pożądanymi warunkami uprawy. Sprawdzić także możliwe ograniczenie sensora AICU przy 2000 ppm.
- Pełen wynik (liczniki i sumy): `agent-control:.agent/results/la-bloomml-wur-kaggle-benchmark-20261104-v2.json` i `la-bloomml-wur-qc-extremes-20261104-v1.json`. Kaggle mirror downloaded by Mac Local Agent into **katalog tymczasowy**, archiwum usunięte po audycie; nie commitowano surowych danych.
- **Testy:** 10/10 importer/ZIP/Excel-time/schemat SHA/missing/holdout PASS; 5/5 forecast transfer PASS; dodany 1 test ostrzeżeń skrajnych; ostateczny **16/16 PASS** na dokładnym checkout SHA `06b901f2b54d3e967756fb45c2a3cec7a6be3b5b` (Mac Python stdlib). Nie uruchamiano ponownie pełnego 63-testowego pakietu fizyki ani nowego eksperymentu GPU.

## Benchmark: prognoza 60 min na ostatnich 20% dni

**Persistence:** prognoza `y(t+60min)=y(t)`; ocena tylko na odłożonych dniach, bez interpolacji. Każda komora ma 9505 odłożonych znaczników, ok. 9492 poprawnych par dla prognozy godzinnej. 5 i 15 min również są policzone w raporcie narzędzia. 1 minuta przy próbkowaniu 5-minutowym pozostaje **unsupported**.

| Komora | T MAE, °C | RH MAE, pp | CO₂ MAE, ppm | PAR MAE, w źródłowej jednostce |
| --- | ---: | ---: | ---: | ---: |
| AICU | 0,969 | 4,134 | 51,882 | 79,909 |
| Automatoes | 1,075 | 2,956 | 57,446 | 77,335 |
| Digilog | 1,046 | 3,653 | 45,710 | 86,202 |
| IUACAAS | 1,165 | 4,360 | 39,355 | 83,170 |
| Reference | 0,898 | 4,031 | 40,640 | 81,786 |
| TheAutomators | 1,028 | 4,144 | 96,140 | 85,666 |

Powyższa tabela to **persistence**, nie wynik modelu fizycznego ani TinyML. Znaczenie/skalowanie PAR do potwierdzenia na podstawie oryginalnej dokumentacji; na razie jest to tylko jednostka podana przez źródło.

## Pierwszy prawdziwy test *transferu* (mały, uczony model)

Narzędzie `tools/ml/wur_forecast_probe.py` uczy **jedną stałą a** z wcześniejszych 80% dni na pięciu *innych* szklarniach. Model: `clip(y(t)+a·(y(t)-y(t−5min)))`. Szósta szklarnia jest **całkowicie niewidziana podczas dopasowania współczynnika**, a test obejmuje jej ostatnie 20% dni; brak przyszłych wartości pogody, nastaw, światła lub celu w cechach. `a` estymowane przez OLS i ograniczone do [−4,4], bez strojenia na szklarniach testowych. Porównanie model↔persistence dla **dokładnie tych samych par testowych**. Przebieg `agent-control:.agent/results/la-bloomml-wur-unseen-household-forecast-20261104-v2.json`, SHA `65d226abe29e4a8bddb0865156aa7a345b902ecb`, **15/15 wtedy PASS**.

| Komora | T MAE persistence → trend (60 min) | CO₂ MAE persistence → trend (60 min) |
| --- | --- | --- |
| AICU | 0,969 → **0,988** (gorzej 2,0%) | 51,88 → **50,78** (lepiej 2,1%) |
| Automatoes | 1,075 → **1,084** (gorzej 0,8%) | 57,45 → **57,27** (lepiej 0,3%) |
| Digilog | 1,047 → **1,090** (gorzej 4,1%) | 45,71 → **45,65** (lepiej 0,1%) |
| IUACAAS | 1,165 → **1,240** (gorzej 6,4%) | 39,36 → **39,28** (lepiej 0,2%) |
| Reference | 0,898 → **0,917** (gorzej 2,1%) | 40,64 → **40,35** (lepiej 0,7%) |
| TheAutomators | 1,028 → **1,063** (gorzej 3,5%) | 96,14 → **96,38** (gorzej 0,2%) |

**Wniosek negatywny, ważny:** samo uczenie trendu CO₂/T/RH na innych szklarniach nie gwarantuje lepszej prognozy. Model **przegrywa dla T60 we wszystkich sześciu** komorach; niewielka poprawa CO₂ nie jest wystarczającą kwalifikacją. Nie wolno podmieniać tego wyniku na sukces. Kolejną hipotezą są przeszłe i bieżące wymuszenia `Tout,PARout`, lampy, dosowanie CO₂ i rzeczywiste sygnały wentylacji, ale dopiero po weryfikacji ich znaczenia i bez używania przyszłych stanów.

## Reprodukcja (Mac lub sandbox z publicznym ZIP)

```bash
python3 -m unittest -q tests.test_wur_external_benchmark tests.test_wur_forecast_probe
kaggle datasets download -d piantic/autonomous-greenhouse-challengeagc-2nd-2019 -p /tmp/wur
python3 -m tools.ml.wur_external_benchmark --zip /tmp/wur/autonomous-greenhouse-challengeagc-2nd-2019.zip --output /tmp/wur/audit.json
python3 -m tools.ml.wur_forecast_probe --zip /tmp/wur/autonomous-greenhouse-challengeagc-2nd-2019.zip --output /tmp/wur/forecast.json
```

Domyślnie kod **wymaga dokładnego SHA256 ZIP**. Do niezależnego archiwum musi powstać **nowy odrębnie zweryfikowany kontrakt**; nie wyłączać pinu bez udokumentowania nowej wersji.

## Stan i następne bramy

**Dla badań metod prognostycznych zewnętrznych szklarni:** TAK, w zakresie obserwowanych sygnałów, z warnings i warunkową licencją. **Dla właściwej fizycznej walidacji naszego growboxa:** NIE. **Dla sterowania i treningu produkcyjnego:** NIE. Oryginalny WUR archive i semantyka czujników/aktuatorów pozostają do potwierdzenia.

Następny krok: odzyskać oficjalne 7z (4TU API/strona) i porównać *wewnętrzne* pliki, nie MD5 kontenera ZIP z 7z; zweryfikować PDF dokumentacji i jednostki. Następnie czyste cechy wymuszeń znane w momencie prognozy, test CO₂/T/RH 5/15/60 min na niewidzianej szklarni, benchmark persistence i RMSE/MAE/bias; porównać również fizyczny solver pod tymi samymi wymuszeniami, kiedy dane umożliwią zmapowanie parametrów. Nie trenować przy pomocy przyszłych stanów realizacji.
