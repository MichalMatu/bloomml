from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/climate/input/sensors/Scd41InsideSource.cpp"


def _begin_body() -> str:
    source = SOURCE.read_text()
    begin = source.index("bool Scd41InsideSource::begin")
    end = source.index("bool Scd41InsideSource::fillCached", begin)
    return source[begin:end]


def test_scd41_clean_start_sequence_is_preserved() -> None:
    body = _begin_body()
    calls = (
        "scd4x_wake_up()",
        "scd4x_stop_periodic_measurement()",
        "scd4x_reinit()",
        "scd4x_start_periodic_measurement()",
    )
    positions = [body.index(call) for call in calls]
    assert positions == sorted(positions)


def test_scd41_availability_depends_on_periodic_start_result() -> None:
    body = _begin_body()
    start = body.index("const int16_t error = scd4x_start_periodic_measurement()")
    availability = body.index("available_ = error == 0", start)
    started = body.index("started_ = available_", availability)
    returned = body.index("return available_", started)
    assert start < availability < started < returned
