"""Browser-origin and request-size regression checks for the serial command panel."""

from __future__ import annotations

import json
import threading
import urllib.error
import urllib.request
from http.server import ThreadingHTTPServer

import pytest

from tools.panel import server


@pytest.fixture
def command_server(monkeypatch):
    class RecordingBridge:
        def __init__(self):
            self.commands = []
            self.diagnostic_refreshes = 0

        def send_command(self, command):
            self.commands.append(command)

        def snapshot(self):
            return {"connected": True}

        def request_diagnostics(self):
            self.diagnostic_refreshes += 1
            return {"type": "diagnostics"}

        def diagnostics_snapshot(self):
            return {"connected": True, "refreshes": self.diagnostic_refreshes}

    bridge = RecordingBridge()
    monkeypatch.setattr(server, "BRIDGE", bridge)
    httpd = ThreadingHTTPServer(("127.0.0.1", 0), server.PanelHandler)
    worker = threading.Thread(target=httpd.serve_forever, daemon=True)
    worker.start()
    try:
        yield bridge, f"http://127.0.0.1:{httpd.server_port}"
    finally:
        httpd.shutdown()
        httpd.server_close()
        worker.join(timeout=3)


def post_command(base: str, *, headers=None, raw=None, path="/api/command"):
    payload = raw if raw is not None else json.dumps({"command": "pause"}).encode("utf-8")
    req = urllib.request.Request(
        f"{base}{path}", data=payload, method="POST",
        headers={"Content-Type": "application/json", **(headers or {})},
    )
    try:
        with urllib.request.urlopen(req, timeout=3) as response:
            return response.status, json.loads(response.read())
    except urllib.error.HTTPError as error:
        return error.code, json.loads(error.read())


@pytest.mark.parametrize("origin", ["https://evil.example", "null", "http://evil.example"])
def test_rejects_cross_origin_command_without_touching_bridge(command_server, origin):
    bridge, base = command_server
    status, payload = post_command(base, headers={"Origin": origin})
    assert status == 403
    assert payload["error"] == "cross_origin_request_rejected"
    assert bridge.commands == []


def test_rejects_cross_site_fetch_metadata_without_origin(command_server):
    bridge, base = command_server
    status, _ = post_command(base, headers={"Sec-Fetch-Site": "cross-site"})
    assert status == 403
    assert bridge.commands == []


def test_allows_same_origin_browser_and_originless_cli(command_server):
    bridge, base = command_server
    status, _ = post_command(base, headers={"Origin": base, "Sec-Fetch-Site": "same-origin"})
    assert status == 200
    status, _ = post_command(base)
    assert status == 200
    assert bridge.commands == [{"command": "pause"}, {"command": "pause"}]


def test_rejects_simple_form_content_type(command_server):
    bridge, base = command_server
    status, payload = post_command(base, headers={"Content-Type": "text/plain"})
    assert status == 400
    assert "Content-Type" in payload["error"]
    assert bridge.commands == []


def test_rejects_oversized_body_before_forwarding_command(command_server):
    bridge, base = command_server
    large = b" " * (server.MAX_JSON_BODY_BYTES + 1)
    status, payload = post_command(base, raw=large)
    assert status == 400
    assert "size" in payload["error"]
    assert bridge.commands == []


def test_rejects_dns_rebinding_host_header(command_server):
    bridge, base = command_server
    status, payload = post_command(base, headers={"Host": "attacker.example"})
    assert status == 403
    assert payload["error"] == "cross_origin_request_rejected"
    assert bridge.commands == []


def test_diagnostics_get_cannot_send_serial_refresh(command_server):
    bridge, base = command_server
    for path, expected in [("/api/diagnostics", 200), ("/api/diagnostics?refresh=1", 405)]:
        try:
            with urllib.request.urlopen(f"{base}{path}", timeout=3) as response:
                status = response.status
        except urllib.error.HTTPError as error:
            status = error.code
        assert status == expected
    assert bridge.diagnostic_refreshes == 0


@pytest.mark.parametrize("headers", [
    {"Origin": "https://evil.example"},
    {"Sec-Fetch-Site": "cross-site"},
])
def test_rejects_cross_site_diagnostics_refresh(command_server, headers):
    bridge, base = command_server
    status, payload = post_command(base, path="/api/diagnostics/refresh", headers=headers)
    assert status == 403
    assert payload["error"] == "cross_origin_request_rejected"
    assert bridge.diagnostic_refreshes == 0


def test_same_origin_diagnostics_refresh_requires_post(command_server):
    bridge, base = command_server
    status, payload = post_command(base, path="/api/diagnostics/refresh")
    assert status == 200
    assert payload["refreshes"] == 1
    assert bridge.diagnostic_refreshes == 1


def test_diagnostics_refresh_timeout_is_not_reported_as_success(command_server, monkeypatch):
    bridge, base = command_server
    monkeypatch.setattr(bridge, "request_diagnostics", lambda: None)
    status, payload = post_command(base, path="/api/diagnostics/refresh")
    assert status == 504
    assert payload["error"] == "diagnostics_refresh_timeout"
