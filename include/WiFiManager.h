#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/***********************************************************************
 *
 *                      WiFiManager
 *
 *  Proyecto:
 *      Invernadero Inteligente IoT
 *
 *  Descripción:
 *
 *      Gestiona completamente la conexión WiFi mediante una
 *      máquina de estados NO BLOQUEANTE.
 *
 *      Responsabilidades:
 *          • Inicializar el módulo WiFi.
 *          • Establecer la conexión con la red.
 *          • Mantener la conexión.
 *          • Reconectar automáticamente.
 *          • Proporcionar información del enlace.
 *
 *      Este módulo NO conoce nada acerca de MQTT.
 *
 ***********************************************************************/

#include <Arduino.h>
#include <WiFiS3.h>

#include "Config.h"

/***********************************************************************
 *                  Estados del módulo WiFi
 ***********************************************************************/

enum class WiFiState
{
    DISCONNECTED,       // Sin conexión

    CONNECTING,         // Intentando conectar

    CONNECTED,          // Conectado correctamente

    CONNECTION_FAILED,  // Timeout durante la conexión

    RECONNECTING        // Esperando el siguiente intento
};

/***********************************************************************
 *                      Clase WiFiManager
 ***********************************************************************/

class WiFiManager
{

public:

    /***************************************************************
     * Constructor
     ***************************************************************/
    WiFiManager();

    /***************************************************************
     * Inicializa el módulo.
     *
     * Debe llamarse únicamente una vez desde setup().
     ***************************************************************/
    void begin();

    /***************************************************************
     * Actualiza la máquina de estados.
     *
     * Debe llamarse continuamente desde loop().
     ***************************************************************/
    void loop();

    /***************************************************************
     * Indica si existe conexión WiFi.
     ***************************************************************/
    bool isConnected() const;

    /***************************************************************
     * Devuelve el estado actual del módulo.
     ***************************************************************/
    WiFiState getState() const;

    /***************************************************************
     * Devuelve la dirección IP asignada.
     ***************************************************************/
    IPAddress localIP() const;

    /***************************************************************
     * Devuelve la intensidad de la señal (RSSI).
     *
     * Unidad:
     *      dBm
     ***************************************************************/
    long RSSI() const;

    /***************************************************************
     * Devuelve el número de intentos de conexión realizados.
     *
     * Útil para depuración y diagnóstico.
     ***************************************************************/
    uint32_t getConnectionAttempts() const;

private:

    /***************************************************************
     * Inicia un nuevo intento de conexión.
     *
     * Esta función únicamente ejecuta WiFi.begin().
     * No bloquea la ejecución.
     ***************************************************************/
    void startConnection();

    /***************************************************************
     * Supervisa el intento de conexión iniciado previamente.
     *
     * Comprueba:
     *      • Si la conexión fue exitosa.
     *      • Si ocurrió un timeout.
     ***************************************************************/
    void updateConnection();

    /***************************************************************
     * Comprueba periódicamente si la conexión se perdió.
     ***************************************************************/
    void checkConnection();

    /***************************************************************
     * Desconecta la interfaz WiFi.
     ***************************************************************/
    void disconnect();

    /***************************************************************
     * Imprime información de la conexión.
     ***************************************************************/
    void printConnectionInfo() const;

    /***************************************************************
     * Imprime mensajes de depuración.
     ***************************************************************/
    void printDebug(const String& message) const;

private:

    /***************************************************************
     * Estado actual del módulo.
     ***************************************************************/
    WiFiState currentState;

    /***************************************************************
     * Instante en que comenzó el intento de conexión actual.
     ***************************************************************/
    unsigned long connectionStartTime;

    /***************************************************************
     * Instante del último intento de reconexión.
     ***************************************************************/
    unsigned long lastReconnectAttempt;

    /***************************************************************
     * Contador de intentos de conexión.
     ***************************************************************/
    uint32_t connectionAttempt;

};

#endif