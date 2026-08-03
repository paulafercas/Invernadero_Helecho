import threading
import time
from copy import deepcopy
from typing import Any, Dict, Optional

from mqtt_manager import MQTTManager
from topics import (
    TOPIC_CMD_BOMBILLA,
    TOPIC_CMD_HUMIDIFICADOR,
    TOPIC_CMD_MODO,
    TOPIC_CMD_SETPOINT,
    TOPIC_CMD_VENTILADOR,
    TOPIC_SENSOR_HUMEDAD,
    TOPIC_SENSOR_NIVEL,
    TOPIC_SENSOR_TEMPERATURA,
    TOPIC_STATE_ACTUADORES,
    TOPIC_STATE_BOMBILLA,
    TOPIC_STATE_HUMIDIFICADOR,
    TOPIC_STATE_MODO,
    TOPIC_STATE_SETPOINT,
    TOPIC_STATE_VENTILADOR,
    TOPIC_SUBSCRIBE_ALL,
    TOPIC_SYSTEM_ALARMA,
)
from config import get_config

_STATE: Dict[str, Any] = {
    "temperatura": None,
    "humedad": None,
    "nivel_agua": None,
    "ventilador": "OFF",
    "bombilla": "OFF",
    "humidificador": "OFF",
    "modo": "AUTO",
    "setpoint": 70,
    "alarma": "",
    "connected": False,
    "connection_status": "Desconectado",
    "last_update": None,
}

_STATE_LOCK = threading.Lock()
_MQTT_MANAGER: Optional[MQTTManager] = None
_INITIALIZED = False


def _set_state_value(key: str, value: Any) -> None:
    with _STATE_LOCK:
        _STATE[key] = value
        _STATE["last_update"] = time.time()


def _parse_numeric(payload: str) -> Optional[float]:
    try:
        return float(payload)
    except (TypeError, ValueError):
        return None


def _parse_actuators(payload: str) -> None:
    parts = [item.strip() for item in payload.split(";") if item.strip()]
    for item in parts:
        if "=" not in item:
            continue
        name, value = item.split("=", 1)
        normalized = name.strip().lower()
        if normalized in {"ventilador", "bombilla", "humidificador"}:
            _set_state_value(normalized, value.strip().upper())


def _handle_message(topic: str, payload: str) -> None:
    decoded_payload = payload.strip()

    if topic == TOPIC_SENSOR_TEMPERATURA:
        numeric_value = _parse_numeric(decoded_payload)
        if numeric_value is not None:
            _set_state_value("temperatura", numeric_value)
    elif topic == TOPIC_SENSOR_HUMEDAD:
        numeric_value = _parse_numeric(decoded_payload)
        if numeric_value is not None:
            _set_state_value("humedad", numeric_value)
    elif topic == TOPIC_SENSOR_NIVEL:
        numeric_value = _parse_numeric(decoded_payload)
        if numeric_value is not None:
            _set_state_value("nivel_agua", numeric_value)
    elif topic == TOPIC_STATE_ACTUADORES:
        _parse_actuators(decoded_payload)
    elif topic == TOPIC_STATE_BOMBILLA:
        _set_state_value("bombilla", decoded_payload.upper())
    elif topic == TOPIC_STATE_VENTILADOR:
        _set_state_value("ventilador", decoded_payload.upper())
    elif topic == TOPIC_STATE_HUMIDIFICADOR:
        _set_state_value("humidificador", decoded_payload.upper())
    elif topic == TOPIC_STATE_MODO:
        _set_state_value("modo", decoded_payload.upper())
    elif topic == TOPIC_STATE_SETPOINT:
        numeric_value = _parse_numeric(decoded_payload)
        if numeric_value is not None:
            _set_state_value("setpoint", int(numeric_value))
    elif topic == TOPIC_SYSTEM_ALARMA:
        _set_state_value("alarma", decoded_payload)


def on_connect(client, userdata, flags, rc, properties=None):
    if rc != 0:
        _set_state_value("connection_status", f"Error de conexión: {rc}")
        return

    _set_state_value("connected", True)
    _set_state_value("connection_status", "Conectado")


def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode("utf-8", errors="ignore")
    _handle_message(topic, payload)


def initialize_backend() -> MQTTManager:
    global _MQTT_MANAGER, _INITIALIZED

    if _INITIALIZED and _MQTT_MANAGER is not None:
        return _MQTT_MANAGER

    config = get_config()
    _MQTT_MANAGER = MQTTManager(
        broker=config["broker"],
        port=config["port"],
        username=config["username"],
        password=config["password"],
        client_id=config["client_id"],
        on_connect_callback=on_connect,
        on_message_callback=on_message,
        subscription_topics=[TOPIC_SUBSCRIBE_ALL],
    )
    _MQTT_MANAGER.start()
    _INITIALIZED = True
    return _MQTT_MANAGER


def get_state_snapshot() -> Dict[str, Any]:
    with _STATE_LOCK:
        return deepcopy(_STATE)


def publish_command(actuator: str, command: str) -> bool:
    manager = initialize_backend()
    topic_map = {
        "bombilla": TOPIC_CMD_BOMBILLA,
        "ventilador": TOPIC_CMD_VENTILADOR,
        "humidificador": TOPIC_CMD_HUMIDIFICADOR,
    }

    topic = topic_map.get(actuator.lower())
    if topic is None:
        return False
    return manager.publish(topic, str(command).upper())


def set_mode(mode: str) -> bool:
    return initialize_backend().publish(TOPIC_CMD_MODO, str(mode).upper())


def set_setpoint(value: int) -> bool:
    return initialize_backend().publish(TOPIC_CMD_SETPOINT, str(int(value)))


def shutdown_backend() -> None:
    global _MQTT_MANAGER, _INITIALIZED

    if _MQTT_MANAGER is not None:
        _MQTT_MANAGER.stop()
    _MQTT_MANAGER = None
    _INITIALIZED = False
    _set_state_value("connected", False)
    _set_state_value("connection_status", "Desconectado")
