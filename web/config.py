import streamlit as st


def get_config() -> dict:
    """Lee la configuración desde Streamlit secrets sin exponer credenciales en el código."""
    secrets = st.secrets

    return {
        "broker": secrets.get("MQTT", {}).get("BROKER", "broker.emqx.io"),
        "port": int(secrets.get("MQTT", {}).get("PORT", 1883)),
        "username": secrets.get("MQTT", {}).get("USERNAME", ""),
        "password": secrets.get("MQTT", {}).get("PASSWORD", ""),
        "client_id": secrets.get("MQTT", {}).get("CLIENT_ID", "invernadero_streamlit"),
    }
