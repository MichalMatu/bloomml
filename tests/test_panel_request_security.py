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

        def send_command(self, command):
            self.commands.append(command)

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


def post_command(base: str, *, headers=None, raw=None):
    payload = raw if raw is not None else json.dumps({"command": "pause"}).encode("utf-8")
    req = urllib.request.Request(
        f"{base}/api/command", data=payload, method="POST",
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
