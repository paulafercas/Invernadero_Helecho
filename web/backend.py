import json
import threading
import time
from collections import deque
from pathlib import Path

import paho.mqtt.client as mqtt
from flask import Flask, jsonify, render_template_string, request

app = Flask(__name__)

BASE_DIR = Path(__file__).resolve().parent
FRONTEND_HTML = (BASE_DIR / "frontend.html").read_text(encoding="utf-8")

BROKER_HOST = "broker.emqx.io"
BROKER_PORT = 1883
CLIENT_ID = "invernadero_web_01"

ROOT_TOPIC = "invernadero"

state = {
    "temperatura": None,
    "humedad": None,
    "nivel_agua": None,
    "temperatura_uncertainty": 0.35,
    "humedad_uncertainty": 2.0,
    "nivel_agua_uncertainty": 1.0,
    "ventilador": "OFF",
    "bombilla": "OFF",
    "humidificador": "OFF",
    "modo": "AUTO",
    "setpoint": 70,
    "alarma": "",
    "last_update": None,
}

history = deque(maxlen=40)

mqtt_client = None
mqtt_lock = threading.Lock()


def on_connect(client, userdata, flags, rc, properties=None):
    if rc != 0:
        app.logger.error("MQTT connection failed with rc=%s", rc)
        return
    app.logger.info("MQTT connected, subscribing to %s/#", ROOT_TOPIC)
    client.subscribe(f"{ROOT_TOPIC}/#")


def on_message(client, userdata, msg):
    global state
    topic = msg.topic
    payload = msg.payload.decode("utf-8", errors="ignore")

    with mqtt_lock:
        if topic.endswith("temperatura"):
            try:
                state["temperatura"] = float(payload)
            except ValueError:
                pass
        elif topic.endswith("humedad"):
            try:
                state["humedad"] = float(payload)
            except ValueError:
                pass
        elif topic.endswith("nivel_agua"):
            try:
                state["nivel_agua"] = float(payload)
            except ValueError:
                pass
        elif topic.endswith("actuadores"):
            parts = payload.split(";")
            for item in parts:
                if "=" in item:
                    key, value = item.split("=", 1)
                    key = key.strip().lower()
                    if key in {"ventilador", "bombilla", "humidificador"}:
                        state[key] = value.upper()
        elif topic.endswith("ventilador"):
            state["ventilador"] = payload.upper()
        elif topic.endswith("bombilla"):
            state["bombilla"] = payload.upper()
        elif topic.endswith("humidificador"):
            state["humidificador"] = payload.upper()
        elif topic.endswith("modo"):
            state["modo"] = payload.upper()
        elif topic.endswith("setpoint"):
            try:
                state["setpoint"] = int(float(payload))
            except ValueError:
                pass
        elif topic.endswith("alarma"):
            state["alarma"] = payload

        state["last_update"] = time.time()
        history.append({
            "timestamp": time.time(),
            "temperatura": state["temperatura"],
            "humedad": state["humedad"],
            "nivel_agua": state["nivel_agua"],
            "modo": state["modo"],
        })


def connect_mqtt():
    global mqtt_client
    mqtt_client = mqtt.Client(client_id=CLIENT_ID, callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect(BROKER_HOST, BROKER_PORT, 60)
    mqtt_client.loop_start()


@app.route("/")
def index():
    return render_template_string(FRONTEND_HTML)


@app.route("/api/state")
def api_state():
    with mqtt_lock:
        payload = dict(state)
    return jsonify(payload)


@app.route("/api/control/<action>", methods=["POST"])
def api_control(action):
    data = {}
    try:
        data = json.loads(request.get_data(as_text=True) or "{}")
    except Exception:
        data = {}

    if action == "modo":
        mode = data.get("mode", "AUTO").upper()
        if mqtt_client is not None:
            mqtt_client.publish(f"{ROOT_TOPIC}/configuracion/modo", mode)
        return jsonify({"ok": True, "mode": mode})

    if action == "setpoint":
        value = int(data.get("value", 70))
        if mqtt_client is not None:
            mqtt_client.publish(f"{ROOT_TOPIC}/configuracion/setpoint", str(value))
        return jsonify({"ok": True, "value": value})

    if action == "actuador":
        name = data.get("name", "").lower()
        cmd = data.get("command", "OFF").upper()
        if name in {"ventilador", "bombilla", "humidificador"}:
            if mqtt_client is not None:
                mqtt_client.publish(f"{ROOT_TOPIC}/comandos/{name}", cmd)
            return jsonify({"ok": True, "name": name, "command": cmd})

    return jsonify({"ok": False, "error": "Acción no válida"}), 400


if __name__ == "__main__":
    connect_mqtt()
    app.run(host="0.0.0.0", port=5000, debug=True)
