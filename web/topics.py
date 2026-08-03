ROOT_TOPIC = "invernadero"

TOPIC_SENSOR_TEMPERATURA = f"{ROOT_TOPIC}/sensores/temperatura"
TOPIC_SENSOR_HUMEDAD = f"{ROOT_TOPIC}/sensores/humedad"
TOPIC_SENSOR_NIVEL = f"{ROOT_TOPIC}/sensores/nivel_agua"

TOPIC_CMD_BOMBILLA = f"{ROOT_TOPIC}/comandos/bombilla"
TOPIC_CMD_VENTILADOR = f"{ROOT_TOPIC}/comandos/ventilador"
TOPIC_CMD_HUMIDIFICADOR = f"{ROOT_TOPIC}/comandos/humidificador"
TOPIC_CMD_MODO = f"{ROOT_TOPIC}/configuracion/modo"
TOPIC_CMD_SETPOINT = f"{ROOT_TOPIC}/configuracion/setpoint"

TOPIC_STATE_ACTUADORES = f"{ROOT_TOPIC}/estado/actuadores"
TOPIC_STATE_BOMBILLA = f"{ROOT_TOPIC}/estado/bombilla"
TOPIC_STATE_VENTILADOR = f"{ROOT_TOPIC}/estado/ventilador"
TOPIC_STATE_HUMIDIFICADOR = f"{ROOT_TOPIC}/estado/humidificador"
TOPIC_STATE_MODO = f"{ROOT_TOPIC}/estado/modo"
TOPIC_STATE_SETPOINT = f"{ROOT_TOPIC}/estado/setpoint"
TOPIC_SYSTEM_ALARMA = f"{ROOT_TOPIC}/sistema/alarma"

TOPIC_SUBSCRIBE_ALL = f"{ROOT_TOPIC}/#"
