import streamlit as st

from backend import (
    get_state_snapshot,
    initialize_backend,
    publish_command,
    set_mode,
    set_setpoint,
    shutdown_backend,
)

try:
    from streamlit_autorefresh import st_autorefresh
except ImportError:  # pragma: no cover - optional dependency in local environments
    st_autorefresh = None


st.set_page_config(page_title="Invernadero remoto", page_icon="🌿", layout="wide")

st.markdown(
    """
    <style>
    div.stButton > button[kind="primary"] {
        background-color: #ef4444;
        color: white;
        border: 1px solid #ef4444;
    }
    div.stButton > button[kind="secondary"] {
        background-color: #22c55e;
        color: white;
        border: 1px solid #22c55e;
    }
    </style>
    """,
    unsafe_allow_html=True,
)

initialize_backend()

if st_autorefresh is not None:
    st_autorefresh(interval=2000, limit=None, key="mqtt-refresh")

state = get_state_snapshot()

st.title("🌿 Sistema de control remoto del invernadero")
st.caption("Interfaz nativa de Streamlit para comunicación MQTT con el Arduino UNO R4 WiFi")

temp_display = f"{state['temperatura'] if state['temperatura'] is not None else 'N/D'} °C ± 0.35 °C"
humidity_display = f"{state['humedad'] if state['humedad'] is not None else 'N/D'} % ± 2 %"
level_display = f"{state['nivel_agua'] if state['nivel_agua'] is not None else 'N/D'} mm ± 1 mm"

col1, col2, col3 = st.columns(3)
with col1:
    st.metric("🌡️ Temperatura", temp_display)
with col2:
    st.metric("💧 Humedad relativa", humidity_display)
with col3:
    st.metric("💧 Nivel de agua", level_display)

st.divider()

status_col, mode_col = st.columns([1, 2])
with status_col:
    st.subheader("Estado del sistema")
    if state["connected"]:
        st.success("MQTT conectado")
    else:
        st.warning("MQTT desconectado")
    st.write(f"Modo actual: {state['modo']}")
    st.write(f"SetPoint: {state['setpoint']}")
    if state.get("alarma"):
        st.warning(f"Alerta: {state['alarma']}")

with mode_col:
    st.subheader("Configuración")
    mode = st.radio("Modo de operación", options=["AUTO", "MANUAL"], horizontal=True, index=0 if state["modo"] == "AUTO" else 1)
    if st.button("Aplicar modo"):
        set_mode(mode)
        st.success(f"Modo actualizado a {mode}")

    if mode == "MANUAL":
        new_setpoint = st.number_input("SetPoint", min_value=0, max_value=100, value=int(state["setpoint"]), step=1)
        if st.button("Aplicar SetPoint"):
            set_setpoint(int(new_setpoint))
            st.success(f"SetPoint actualizado a {int(new_setpoint)}")

st.divider()

st.subheader("Estados de actuadores")
actuator_cols = st.columns(3)
with actuator_cols[0]:
    st.metric("🌬️ Ventilador", state["ventilador"])
with actuator_cols[1]:
    st.metric("💡 Bombilla", state["bombilla"])
with actuator_cols[2]:
    st.metric("💦 Humidificador", state["humidificador"])

if state["nivel_agua"] is not None and state["nivel_agua"] < 7:
    st.error("⚠️ Nivel de agua por debajo de 7 mm. Revise el tanque.")

st.divider()

st.subheader("Control manual")
control_cols = st.columns(3)
with control_cols[0]:
    if st.button("🔴 Encender Bombilla", use_container_width=True, type="primary"):
        set_mode("MANUAL")
        publish_command("bombilla", "ON")
with control_cols[1]:
    if st.button("🟢 Apagar Bombilla", use_container_width=True, type="secondary"):
        set_mode("MANUAL")
        publish_command("bombilla", "OFF")

control_cols = st.columns(3)
with control_cols[0]:
    if st.button("🔴 Encender Ventilador", use_container_width=True, type="primary"):
        set_mode("MANUAL")
        publish_command("ventilador", "ON")
with control_cols[1]:
    if st.button("🟢 Apagar Ventilador", use_container_width=True, type="secondary"):
        set_mode("MANUAL")
        publish_command("ventilador", "OFF")

control_cols = st.columns(3)
with control_cols[0]:
    if st.button("🔴 Encender Humidificador", use_container_width=True, type="primary"):
        set_mode("MANUAL")
        publish_command("humidificador", "ON")
with control_cols[1]:
    if st.button("🟢 Apagar Humidificador", use_container_width=True, type="secondary"):
        set_mode("MANUAL")
        publish_command("humidificador", "OFF")

st.divider()
if st.button("Cerrar conexión MQTT"):
    shutdown_backend()
    st.warning("Conexión MQTT cerrada")
