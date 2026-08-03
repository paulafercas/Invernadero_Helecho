# Aplicación Streamlit para invernadero

Esta carpeta contiene la interfaz Streamlit y la capa MQTT separada para el proyecto del invernadero.

## Estructura

- main.py: interfaz Streamlit
- backend.py: lógica de negocio y comunicación MQTT
- mqtt_manager.py: clase reutilizable para MQTT
- topics.py: constantes de topics MQTT
- config.py: lectura de secretos desde Streamlit

## Despliegue en Streamlit Community Cloud

1. Asegúrate de que el directorio raíz del despliegue sea la carpeta del proyecto.
2. El archivo principal de ejecución debe ser web/main.py.
3. Añade las dependencias de requirements.txt en la raíz del proyecto.
4. Define los secretos en Streamlit Cloud usando el nombre [MQTT].
