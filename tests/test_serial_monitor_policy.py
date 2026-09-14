import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_default_monitor_is_no_reset_and_reset_mode_is_explicit() -> None:
    makefile = (ROOT / "Makefile").read_text()
    assert (
        "monitor: ensure-idf\n\tESP_IDF_MONITOR_NO_RESET=1 $(RUN_IDF) -B $(IDF_BUILD_DIR) $(IDF_PORT_ARGS) monitor"
        in makefile
    )
    assert (
        "monitor-reset: ensure-idf\n\t$(RUN_IDF) -B $(IDF_BUILD_DIR) $(IDF_PORT_ARGS) monitor"
        in makefile
    )
    assert (
        "flash-monitor: build\n\tESP_IDF_MONITOR_NO_RESET=1 $(RUN_IDF) -B $(IDF_BUILD_DIR) $(IDF_PORT_ARGS) flash monitor"
        in makefile
    )


def test_vscode_monitor_defaults_to_no_reset() -> None:
    settings = json.loads((ROOT / ".vscode/settings.json").read_text())
    assert settings["idf.monitorNoReset"] is True
