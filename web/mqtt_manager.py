import logging
import threading
import time
from typing import Callable, List, Optional

import paho.mqtt.client as mqtt


class MQTTManager:
    """Encapsula la conexión MQTT y la gestión del ciclo de mensajes."""

    def __init__(
        self,
        broker: str,
        port: int,
        username: Optional[str] = None,
        password: Optional[str] = None,
        client_id: Optional[str] = None,
        on_connect_callback: Optional[Callable[..., None]] = None,
        on_message_callback: Optional[Callable[..., None]] = None,
        subscription_topics: Optional[List[str]] = None,
    ) -> None:
        self.broker = broker
        self.port = port
        self.username = username
        self.password = password
        self.client_id = client_id or f"streamlit_{int(time.time())}"
        self.on_connect_callback = on_connect_callback
        self.on_message_callback = on_message_callback
        self.subscription_topics = subscription_topics or []

        self.client: Optional[mqtt.Client] = None
        self.connected = False
        self._lock = threading.Lock()
        self._running = False
        self._logger = logging.getLogger(__name__)

    def connect(self) -> bool:
        if self.client is None:
            self.client = mqtt.Client(
                client_id=self.client_id,
                callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            )
            self.client.on_connect = self.on_connect
            self.client.on_message = self.on_message

            if self.username and self.password:
                self.client.username_pw_set(self.username, self.password)

        try:
            self.client.connect(self.broker, self.port, 60)
            self.client.loop_start()
            self.connected = True
            return True
        except Exception as exc:  # pragma: no cover - defensive branch
            self._logger.exception("MQTT connection failed: %s", exc)
            self.connected = False
            return False

    def disconnect(self) -> None:
        if self.client is not None:
            try:
                self.client.loop_stop()
                self.client.disconnect()
            except Exception:  # pragma: no cover - defensive branch
                pass
        self.connected = False

    def reconnect(self) -> bool:
        self.disconnect()
        return self.connect()

    def publish(self, topic: str, payload: str) -> bool:
        if self.client is None:
            self.connect()

        if self.client is None or not self.connected:
            return False

        try:
            self.client.publish(topic, payload)
            return True
        except Exception as exc:  # pragma: no cover - defensive branch
            self._logger.exception("MQTT publish failed: %s", exc)
            return False

    def subscribe(self, topics: Optional[List[str]] = None) -> None:
        topics_to_subscribe = topics or self.subscription_topics
        if self.client is None:
            self.connect()

        if self.client is None:
            return

        for topic in topics_to_subscribe:
            self.client.subscribe(topic)

    def on_message(self, client, userdata, msg) -> None:
        if self.on_message_callback is not None:
            self.on_message_callback(client, userdata, msg)

    def on_connect(self, client, userdata, flags, rc, properties=None) -> None:
        with self._lock:
            self.connected = rc == 0
        if rc == 0:
            self.subscribe(self.subscription_topics)
        if self.on_connect_callback is not None:
            self.on_connect_callback(client, userdata, flags, rc, properties)

    def start(self) -> bool:
        with self._lock:
            if self._running:
                return True
            self._running = True

        return self.connect()

    def stop(self) -> None:
        with self._lock:
            self._running = False
        self.disconnect()
