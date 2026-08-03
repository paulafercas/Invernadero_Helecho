/***********************************************************************
 *
 *                      MQTTManager.cpp
 *
 ***********************************************************************/

#include "Config.h"
#include "Topics.h"
#include "MQTTManager.h"

/***********************************************************************
 *              Instancia estática del callback
 ***********************************************************************/

MQTTManager* MQTTManager::instance = nullptr;

/***********************************************************************
 *                      Constructor
 ***********************************************************************/

MQTTManager::MQTTManager(WiFiManager& wifiManager)
    : wifi(wifiManager),
      client(wifiClient)
{
    currentState = MQTTState::DISCONNECTED;

    connectionStartTime = 0;

    lastReconnectAttempt = 0;

    connectionAttempt = 0;

    this->messageHandler = nullptr;

    instance = this;
}

/***********************************************************************
 *                      begin()
 ***********************************************************************/

void MQTTManager::begin()
{
    client.setServer(MQTT_SERVER, MQTT_PORT);

    client.setCallback(MQTTManager::mqttCallback);

    printDebug("");

    printDebug("====================================");

    printDebug("Inicializando MQTT...");

    printDebug("====================================");

    currentState = MQTTState::DISCONNECTED;
}

/***********************************************************************
 *                      loop()
 ***********************************************************************/

void MQTTManager::loop()
{

    /*--------------------------------------------------------------
        Si el WiFi no está disponible no tiene sentido intentar
        conectar con el broker MQTT.
    --------------------------------------------------------------*/

    if(!wifi.isConnected())
    {
        currentState = MQTTState::DISCONNECTED;

        return;
    }

    switch(currentState)
    {

        case MQTTState::DISCONNECTED:

            startConnection();

            break;

        case MQTTState::CONNECTING:

            updateConnection();

            break;

        case MQTTState::CONNECTED:

            client.loop();

            checkConnection();

            break;

        case MQTTState::CONNECTION_FAILED:

            if(millis() - lastReconnectAttempt >= MQTT_RECONNECT_TIME)
            {
                currentState = MQTTState::RECONNECTING;
            }

            break;

        case MQTTState::RECONNECTING:

            startConnection();

            break;
    }

}

/***********************************************************************
 *                  startConnection()
 ***********************************************************************/

void MQTTManager::startConnection()
{

    printDebug("");

    printDebug("Intentando conectar con el broker MQTT...");

    connectionAttempt++;

    connectionStartTime = millis();

    currentState = MQTTState::CONNECTING;

    bool connected =
        client.connect(
            MQTT_CLIENT_ID,
            MQTT_USER,
            MQTT_PASSWORD
        );

    if(connected)
    {
        currentState = MQTTState::CONNECTED;

        printDebug("Broker MQTT conectado.");

        subscribeTopics();

        return;
    }

}

/***********************************************************************
 *                  updateConnection()
 ***********************************************************************/

void MQTTManager::updateConnection()
{

    if(client.connected())
    {
        currentState = MQTTState::CONNECTED;

        printDebug("Conexion MQTT establecida.");

        subscribeTopics();

        return;
    }

    if(millis() - connectionStartTime >= MQTT_TIMEOUT)
    {

        printDebug("Timeout de conexion MQTT.");

        currentState = MQTTState::CONNECTION_FAILED;

        lastReconnectAttempt = millis();

    }

}

/***********************************************************************
 *                  checkConnection()
 ***********************************************************************/

void MQTTManager::checkConnection()
{
    if(client.connected())
    {
        return;
    }

    printDebug("");

    printDebug("Se perdió la conexión con el broker MQTT.");

    currentState = MQTTState::RECONNECTING;

    lastReconnectAttempt = millis();
}

/***********************************************************************
 *                  subscribeTopics()
 ***********************************************************************/

void MQTTManager::subscribeTopics()
{
    printDebug("Suscribiendo topics MQTT...");

    /***************************************************************
     * Comandos de actuadores
     ***************************************************************/

    subscribe(TOPIC_CMD_BOMBILLA);

    subscribe(TOPIC_CMD_VENTILADOR);

    subscribe(TOPIC_CMD_HUMIDIFICADOR);

    /***************************************************************
     * Modo de operación
     ***************************************************************/

    subscribe(TOPIC_CFG_MODO);

    /***************************************************************
     * SetPoint de humedad
     ***************************************************************/

    subscribe(TOPIC_CFG_SETPOINT);

    printDebug("Suscripciones completadas.");
}

/***********************************************************************
 *                      processMessage()
 ***********************************************************************/

void MQTTManager::setMessageHandler(void (*handler)(const char* topic,
                                                     const char* payload))
{
    this->messageHandler = handler;
}

bool MQTTManager::isConnected() const
{
    return const_cast<PubSubClient&>(client).connected();
}

MQTTState MQTTManager::getState() const
{
    return currentState;
}

uint32_t MQTTManager::getConnectionAttempts() const
{
    return connectionAttempt;
}

void MQTTManager::processMessage(const char* topic,
                                 const char* payload)
{
    if(this->messageHandler != nullptr)
    {
        this->messageHandler(topic, payload);
    }
}

void MQTTManager::printDebug(const String& message) const
{
    if(DEBUG_SERIAL)
    {
        Serial.println(message);
    }
}

/***********************************************************************
 *                      publish()
 ***********************************************************************/

bool MQTTManager::publish(const char* topic,
                          const char* payload,
                          bool retained)
{
    if(!client.connected())
    {
        return false;
    }

    bool success = client.publish(topic,
                                  payload,
                                  retained);

    if(DEBUG_SERIAL)
    {
        Serial.print("[MQTT] Publish -> ");

        Serial.print(topic);

        Serial.print(" : ");

        Serial.println(payload);
    }

    return success;
}

/***********************************************************************
 *                      subscribe()
 ***********************************************************************/

bool MQTTManager::subscribe(const char* topic)
{
    bool success = client.subscribe(topic);

    if(DEBUG_SERIAL)
    {
        Serial.print("[MQTT] Subscribe -> ");

        Serial.print(topic);

        Serial.print(" : ");

        if(success)
            Serial.println("OK");
        else
            Serial.println("ERROR");
    }

    return success;
}

/***********************************************************************
 *                  mqttCallback()
 *
 *      Callback exigido por PubSubClient.
 ***********************************************************************/

void MQTTManager::mqttCallback(char* topic,
                               byte* payload,
                               unsigned int length)
{
    if(instance == nullptr)
    {
        return;
    }

    char message[128];

    if(length >= sizeof(message))
    {
        length = sizeof(message) - 1;
    }

    memcpy(message, payload, length);

    message[length] = '\0';

    if(DEBUG_SERIAL)
    {
        Serial.println();

        Serial.println("========== MQTT RX ==========");

        Serial.print("Topic : ");

        Serial.println(topic);

        Serial.print("Payload : ");

        Serial.println(message);

        Serial.println("=============================");
    }

    instance->processMessage(topic, message);
}