#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

/***********************************************************************
 *
 *                      MQTTManager
 *
 *  Proyecto:
 *      Invernadero Inteligente IoT
 *
 *  Descripción:
 *
 *      Gestiona toda la comunicación MQTT del sistema.
 *
 *      Responsabilidades:
 *
 *          • Conectarse al broker MQTT.
 *          • Reconectarse automáticamente.
 *          • Suscribirse a los topics.
 *          • Publicar mensajes.
 *          • Recibir mensajes.
 *          • Mantener la conexión.
 *
 *      Este módulo NO conoce la lógica de control del invernadero.
 *
 ***********************************************************************/

#include <Arduino.h>
#include <WiFiS3.h>
#include <PubSubClient.h>

#include "Config.h"
#include "Topics.h"
#include "MQTTPayLoads.h"
#include "WiFiManager.h"

/***********************************************************************
 *                  Estados MQTT
 ***********************************************************************/

enum class MQTTState
{
    DISCONNECTED,

    CONNECTING,

    CONNECTED,

    CONNECTION_FAILED,

    RECONNECTING
};

/***********************************************************************
 *                  Clase MQTTManager
 ***********************************************************************/

class MQTTManager
{

public:

    /***************************************************************
     * Constructor
     ***************************************************************/
    explicit MQTTManager(WiFiManager& wifiManager);

    /***************************************************************
     * Inicializa el módulo MQTT.
     ***************************************************************/
    void begin();

    /***************************************************************
     * Debe llamarse continuamente desde loop().
     ***************************************************************/
    void loop();

    /***************************************************************
     * Registra un manejador para los mensajes entrantes.
     ***************************************************************/
    void setMessageHandler(void (*handler)(const char* topic,
                                           const char* payload));

    /***************************************************************
     * Indica si existe conexión con el broker.
     ***************************************************************/
    bool isConnected() const;

    /***************************************************************
     * Devuelve el estado actual del módulo MQTT.
     ***************************************************************/
    MQTTState getState() const;

    /***************************************************************
     * Publica un mensaje.
     ***************************************************************/
    bool publish(const char* topic,
                 const char* payload,
                 bool retained = false);

    /***************************************************************
     * Suscribe un topic.
     ***************************************************************/
    bool subscribe(const char* topic);

    /***************************************************************
     * Devuelve el número de intentos de conexión.
     ***************************************************************/
    uint32_t getConnectionAttempts() const;

private:

    /***************************************************************
     * Inicia un intento de conexión.
     ***************************************************************/
    void startConnection();

    /***************************************************************
     * Actualiza el proceso de conexión.
     ***************************************************************/
    void updateConnection();

    /***************************************************************
     * Comprueba si la conexión continúa activa.
     ***************************************************************/
    void checkConnection();

    /***************************************************************
     * Se suscribe a todos los topics del proyecto.
     ***************************************************************/
    void subscribeTopics();

    /***************************************************************
     * Callback interno de PubSubClient.
     ***************************************************************/
    static void mqttCallback(char* topic,
                             byte* payload,
                             unsigned int length);

    /***************************************************************
     * Procesa el mensaje recibido.
     ***************************************************************/
    void processMessage(const char* topic,
                        const char* payload);

    /***************************************************************
     * Imprime mensajes de depuración.
     ***************************************************************/
    void printDebug(const String& message) const;

private:

    WiFiManager& wifi;

    WiFiClient wifiClient;

    PubSubClient client;

    MQTTState currentState;

    unsigned long connectionStartTime;

    unsigned long lastReconnectAttempt;

    uint32_t connectionAttempt;

    void (*messageHandler)(const char* topic,
                            const char* payload) = nullptr;

    static MQTTManager* instance;

};

#endif