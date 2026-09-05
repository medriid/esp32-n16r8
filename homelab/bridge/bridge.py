import json
import os
import queue
import threading
import time
from copy import deepcopy
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

import paho.mqtt.client as mqtt
import serial


SERIAL_PORT = os.environ.get("SERIAL_PORT", "/dev/ttyACM0")
MQTT_HOST = os.environ.get("MQTT_HOST", "127.0.0.1")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
WEB_HOST = os.environ.get("WEB_HOST", "0.0.0.0")
WEB_PORT = int(os.environ.get("WEB_PORT", "8080"))

BASE_TOPIC = "home/desk-esp32"
RGB_COMMAND_TOPIC = f"{BASE_TOPIC}/rgb/set"
RGB_STATE_TOPIC = f"{BASE_TOPIC}/rgb/state"
EXTERNAL_COMMAND_TOPIC = f"{BASE_TOPIC}/external/set"
EXTERNAL_STATE_TOPIC = f"{BASE_TOPIC}/external/state"
AVAILABILITY_TOPIC = f"{BASE_TOPIC}/availability"

RGB_EFFECTS = {"solid", "blink", "pulse", "rainbow", "off"}
EXTERNAL_EFFECTS = {"solid", "blink", "pulse", "off"}


def bounded_integer(value: Any, low: int, high: int, name: str) -> int:
    if isinstance(value, bool):
        raise ValueError(f"{name} must be a number")
    try:
        number = int(value)
    except (TypeError, ValueError) as error:
        raise ValueError(f"{name} must be a number") from error
    if not low <= number <= high:
        raise ValueError(f"{name} must be between {low} and {high}")
    return number


class SerialLink:
    def __init__(self, on_state, on_connection):
        self.on_state = on_state
        self.on_connection = on_connection
        self.commands: queue.Queue[str] = queue.Queue(maxsize=100)
        self.stop_event = threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self.thread.start()

    def send(self, command: str) -> None:
        if "\n" in command or "\r" in command or len(command) > 120:
            raise ValueError("invalid serial command")
        try:
            self.commands.put_nowait(command)
        except queue.Full as error:
            raise RuntimeError("serial command queue is full") from error

    def _open(self) -> serial.Serial:
        connection = serial.Serial()
        connection.port = SERIAL_PORT
        connection.baudrate = 115200
        connection.timeout = 0.25
        connection.write_timeout = 2
        connection.exclusive = True
        connection.dtr = False
        connection.rts = False
        connection.open()
        return connection

    def _run(self) -> None:
        while not self.stop_event.is_set():
            try:
                with self._open() as connection:
                    print(f"Serial connected: {SERIAL_PORT}", flush=True)
                    self.on_connection(True)
                    connection.write(b"state\n")

                    while not self.stop_event.is_set():
                        while True:
                            try:
                                command = self.commands.get_nowait()
                            except queue.Empty:
                                break
                            connection.write(command.encode("utf-8") + b"\n")
                            print(f"Serial command: {command}", flush=True)

                        raw_line = connection.readline()
                        if not raw_line:
                            continue
                        line = raw_line.decode("utf-8", errors="replace").strip()
                        if line:
                            print(f"ESP32: {line}", flush=True)
                        if not line.startswith("{"):
                            continue
                        try:
                            payload = json.loads(line)
                        except json.JSONDecodeError:
                            continue
                        if payload.get("type") == "state":
                            self.on_state(payload)
            except (OSError, serial.SerialException) as error:
                print(f"Serial unavailable: {error}", flush=True)
                self.on_connection(False)
                self.stop_event.wait(2)


class MqttLink:
    def __init__(self, app):
        self.app = app
        self.connected = False
        self.client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id="esp32-usb-bridge",
        )
        self.client.will_set(AVAILABILITY_TOPIC, "offline", retain=True)
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

    def start(self) -> None:
        self.client.connect_async(MQTT_HOST, MQTT_PORT, keepalive=30)
        self.client.loop_start()

    def _on_connect(self, client, userdata, flags, reason_code, properties) -> None:
        self.connected = not reason_code.is_failure
        if not self.connected:
            print(f"MQTT connection failed: {reason_code}", flush=True)
            return
        print(f"MQTT connected: {MQTT_HOST}:{MQTT_PORT}", flush=True)
        client.subscribe([(RGB_COMMAND_TOPIC, 0), (EXTERNAL_COMMAND_TOPIC, 0)])
        self.publish_discovery()
        self.publish_state(self.app.snapshot())

    def _on_disconnect(
        self, client, userdata, disconnect_flags, reason_code, properties
    ) -> None:
        self.connected = False
        print(f"MQTT disconnected: {reason_code}", flush=True)

    def _on_message(self, client, userdata, message) -> None:
        try:
            payload = json.loads(message.payload.decode("utf-8"))
            if not isinstance(payload, dict):
                raise ValueError("command must be a JSON object")
            if message.topic == RGB_COMMAND_TOPIC:
                self.app.apply_rgb(payload)
            elif message.topic == EXTERNAL_COMMAND_TOPIC:
                self.app.apply_external(payload)
        except (UnicodeDecodeError, json.JSONDecodeError, ValueError, RuntimeError) as error:
            print(f"Rejected MQTT command on {message.topic}: {error}", flush=True)

    def publish_discovery(self) -> None:
        device = {
            "identifiers": ["esp32_s3_n16r8_usb"],
            "name": "Desk ESP32-S3",
            "manufacturer": "Espressif",
            "model": "ESP32-S3 N16R8",
        }
        rgb_config = {
            "name": "Onboard RGB",
            "unique_id": "esp32_s3_onboard_rgb",
            "schema": "json",
            "command_topic": RGB_COMMAND_TOPIC,
            "state_topic": RGB_STATE_TOPIC,
            "availability_topic": AVAILABILITY_TOPIC,
            "brightness": True,
            "supported_color_modes": ["rgb"],
            "effect": True,
            "effect_list": ["solid", "blink", "pulse", "rainbow"],
            "device": device,
        }
        external_config = {
            "name": "External red LED",
            "unique_id": "esp32_s3_external_red_led",
            "schema": "json",
            "command_topic": EXTERNAL_COMMAND_TOPIC,
            "state_topic": EXTERNAL_STATE_TOPIC,
            "availability_topic": AVAILABILITY_TOPIC,
            "brightness": True,
            "supported_color_modes": ["brightness"],
            "effect": True,
            "effect_list": ["solid", "blink", "pulse"],
            "device": device,
        }
        self.client.publish(
            "homeassistant/light/esp32_s3/onboard_rgb/config",
            json.dumps(rgb_config, separators=(",", ":")),
            retain=True,
        )
        self.client.publish(
            "homeassistant/light/esp32_s3/external_red/config",
            json.dumps(external_config, separators=(",", ":")),
            retain=True,
        )

    def publish_state(self, snapshot: dict[str, Any]) -> None:
        if not self.connected:
            return
        is_connected = snapshot["connected"]
        self.client.publish(
            AVAILABILITY_TOPIC,
            "online" if is_connected else "offline",
            retain=True,
        )
        if not is_connected:
            return

        rgb = snapshot["rgb"]
        rgb_payload = {
            "state": "OFF" if rgb["effect"] == "off" else "ON",
            "brightness": rgb["brightness"],
            "color_mode": "rgb",
            "color": {"r": rgb["red"], "g": rgb["green"], "b": rgb["blue"]},
            "effect": rgb["effect"],
        }
        external = snapshot["external"]
        external_payload = {
            "state": "OFF" if external["effect"] == "off" else "ON",
            "brightness": external["brightness"],
            "color_mode": "brightness",
            "effect": external["effect"],
        }
        self.client.publish(
            RGB_STATE_TOPIC,
            json.dumps(rgb_payload, separators=(",", ":")),
            retain=True,
        )
        self.client.publish(
            EXTERNAL_STATE_TOPIC,
            json.dumps(external_payload, separators=(",", ":")),
            retain=True,
        )


class BridgeApp:
    def __init__(self):
        self.lock = threading.Lock()
        self.state: dict[str, Any] = {
            "connected": False,
            "last_seen": None,
            "rgb": {
                "effect": "unknown",
                "red": 0,
                "green": 0,
                "blue": 0,
                "brightness": 0,
                "period": 1000,
            },
            "external": {
                "effect": "unknown",
                "brightness": 0,
                "period": 1000,
            },
        }
        self.serial = SerialLink(self._on_serial_state, self._on_serial_connection)
        self.mqtt = MqttLink(self)

    def start(self) -> None:
        self.serial.start()
        self.mqtt.start()

    def snapshot(self) -> dict[str, Any]:
        with self.lock:
            return deepcopy(self.state)

    def _on_serial_connection(self, connected: bool) -> None:
        with self.lock:
            self.state["connected"] = connected
        self.mqtt.publish_state(self.snapshot())

    def _on_serial_state(self, payload: dict[str, Any]) -> None:
        rgb = payload.get("rgb")
        external = payload.get("external")
        if not isinstance(rgb, dict) or not isinstance(external, dict):
            return
        with self.lock:
            self.state["connected"] = True
            self.state["last_seen"] = int(time.time())
            self.state["rgb"] = rgb
            self.state["external"] = external
        self.mqtt.publish_state(self.snapshot())

    def _queue(self, commands: list[str]) -> None:
        for command in commands:
            self.serial.send(command)
        self.serial.send("state")

    def apply_rgb(self, payload: dict[str, Any]) -> None:
        commands: list[str] = []
        color = payload.get("color")
        if isinstance(color, dict):
            red = bounded_integer(color.get("r"), 0, 255, "red")
            green = bounded_integer(color.get("g"), 0, 255, "green")
            blue = bounded_integer(color.get("b"), 0, 255, "blue")
            commands.append(f"rgb color {red} {green} {blue}")
        elif isinstance(color, str):
            value = color.removeprefix("#")
            if len(value) != 6:
                raise ValueError("color must be a six-digit hex value")
            try:
                red, green, blue = (int(value[index:index + 2], 16) for index in (0, 2, 4))
            except ValueError as error:
                raise ValueError("color must be a six-digit hex value") from error
            commands.append(f"rgb color {red} {green} {blue}")
        if "brightness" in payload:
            brightness = bounded_integer(payload["brightness"], 0, 255, "brightness")
            commands.append(f"rgb brightness {brightness}")
        if "period" in payload:
            period = bounded_integer(payload["period"], 100, 10000, "period")
            commands.append(f"rgb speed {period}")

        effect = payload.get("effect")
        state = str(payload.get("state", "")).upper()
        if state == "OFF":
            effect = "off"
        elif effect is None and state == "ON":
            current_effect = self.snapshot()["rgb"]["effect"]
            effect = "solid" if current_effect in {"off", "unknown"} else current_effect
        if effect is not None:
            effect = str(effect).lower()
            if effect not in RGB_EFFECTS:
                raise ValueError("unsupported RGB effect")
            commands.append(f"rgb effect {effect}")

        if not commands:
            raise ValueError("no supported RGB setting was supplied")
        self._queue(commands)

    def apply_external(self, payload: dict[str, Any]) -> None:
        commands: list[str] = []
        if "brightness" in payload:
            brightness = bounded_integer(payload["brightness"], 0, 255, "brightness")
            commands.append(f"external brightness {brightness}")
        if "period" in payload:
            period = bounded_integer(payload["period"], 100, 10000, "period")
            commands.append(f"external speed {period}")

        effect = payload.get("effect")
        state = str(payload.get("state", "")).upper()
        if state == "OFF":
            effect = "off"
        elif effect is None and state == "ON":
            current_effect = self.snapshot()["external"]["effect"]
            effect = "solid" if current_effect in {"off", "unknown"} else current_effect
        if effect is not None:
            effect = str(effect).lower()
            if effect not in EXTERNAL_EFFECTS:
                raise ValueError("unsupported external LED effect")
            commands.append(f"external effect {effect}")

        if not commands:
            raise ValueError("no supported external LED setting was supplied")
        self._queue(commands)


APP = BridgeApp()
INDEX_HTML = Path(__file__).with_name("index.html").read_bytes()


class RequestHandler(BaseHTTPRequestHandler):
    server_version = "ESP32Bridge/1.0"

    def _send_json(self, status: int, payload: dict[str, Any]) -> None:
        body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(INDEX_HTML)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(INDEX_HTML)
        elif self.path == "/api/state":
            self._send_json(200, APP.snapshot())
        elif self.path == "/healthz":
            state = APP.snapshot()
            self._send_json(200 if state["connected"] else 503, state)
        else:
            self._send_json(404, {"error": "not found"})

    def do_POST(self) -> None:
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > 4096:
                raise ValueError("request body must be between 1 and 4096 bytes")
            payload = json.loads(self.rfile.read(length).decode("utf-8"))
            if not isinstance(payload, dict):
                raise ValueError("request must contain a JSON object")
            if self.path == "/api/rgb":
                APP.apply_rgb(payload)
            elif self.path == "/api/external":
                APP.apply_external(payload)
            else:
                self._send_json(404, {"error": "not found"})
                return
            self._send_json(202, {"accepted": True})
        except (UnicodeDecodeError, json.JSONDecodeError, ValueError, RuntimeError) as error:
            self._send_json(400, {"error": str(error)})

    def log_message(self, format_string: str, *args: Any) -> None:
        print(f"HTTP {self.address_string()}: {format_string % args}", flush=True)


if __name__ == "__main__":
    APP.start()
    server = ThreadingHTTPServer((WEB_HOST, WEB_PORT), RequestHandler)
    print(f"Control page listening on {WEB_HOST}:{WEB_PORT}", flush=True)
    server.serve_forever()

