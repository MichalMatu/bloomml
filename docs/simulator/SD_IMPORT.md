# Import pomiarow BloomML z karty SD

Narzędzie hostowe `tools/ml/stage27_sd_import.py` odczytuje istniejące pliki
`GBLOG/*.JL` ze sterownika BloomML (`growbox-log-v3`). Nie modyfikuje
firmware, karty SD, danych zrodlowych ani kodu Growclip.

```bash
python -m tools.ml.stage27_sd_import /sciezka/do/karty/GBLOG --out data/stage27-import
python -m pytest tests/test_stage27_sd_import.py -q
```

Importer zapisuje pochodzenie kazdego pomiaru (SHA-256 pliku, sesja,
wersja firmware, numer linii i czas), odrzuca uszkodzone/nieaktualne
odczyty SCD41 oraz niemonotoniczne znaczniki czasu. Przetwarza duze
pliki liniowo, bez wczytywania calego logu do pamieci. Powtorzony
import tego samego pliku nadpisuje identyczny plik wynikowy zamiast
tworzyc duplikaty. Dane wynikowe i `import_manifest.json` trafiaja do
`data/`, ignorowanego przez Git. Nie publikowac surowych logow
urzadzen bez przegladu danych wrazliwych.

Odczyty wewnetrzne z SCD41, pomocnicze pomiary BLE i komendy
sterownika sa zapisywane oddzielnie. Komendy `c` i telemetria `out`
nie sa dowodem fizycznego wlaczenia urzadzenia. Odczyt BLE nie jest
traktowany jako temperatura na zewnatrz bez potwierdzonej lokalizacji.
Pliki Stage27 nie sa zamienne z `climate-runtime-trace.v1`, ktory
sluzy do odtwarzania decyzji polityki.

**Wazne:** importer nie tworzy jeszcze gotowego zbioru treningowego
ani nie przeprowadza kalibracji termodynamiki. Brakuje potwierdzonych
warunkow zewnetrznych i mocy/przeplywow urzadzen. Nastepny etap to
synchronizacja pomiarow fizycznych, dopasowanie parametrow symulatora
climate-v6 i walidacja na odlozonych w czasie seriach, przed Kaggle.
