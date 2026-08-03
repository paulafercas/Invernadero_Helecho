/***********************************************************************
 *
 *                      WiFiManager.cpp
 *
 ***********************************************************************/

#include "WiFiManager.h"

/***********************************************************************
 *                      Constructor
 ***********************************************************************/

WiFiManager::WiFiManager()
{
    currentState = WiFiState::DISCONNECTED;

    connectionStartTime = 0;

    lastReconnectAttempt = 0;

    connectionAttempt = 0;
}

/***********************************************************************
 *                      begin()
 ***********************************************************************/

void WiFiManager::begin()
{
    printDebug("");
    printDebug("====================================");
    printDebug("Inicializando modulo WiFi...");
    printDebug("====================================");

    currentState = WiFiState::DISCONNECTED;
}

/***********************************************************************
 *                      loop()
 ***********************************************************************/

void WiFiManager::loop()
{
    switch(currentState)
    {

        case WiFiState::DISCONNECTED:

            startConnection();

            break;

        case WiFiState::CONNECTING:

            updateConnection();

            break;

        case WiFiState::CONNECTED:

            checkConnection();

            break;

        case WiFiState::CONNECTION_FAILED:

            if(millis() - lastReconnectAttempt >= WIFI_RECONNECT_TIME)
            {
                currentState = WiFiState::RECONNECTING;
            }

            break;

        case WiFiState::RECONNECTING:

            startConnection();

            break;
    }
}

/***********************************************************************
 *                      startConnection()
 ***********************************************************************/

void WiFiManager::startConnection()
{
    printDebug("");

    printDebug("Intentando conectar al WiFi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    connectionAttempt++;

    connectionStartTime = millis();

    currentState = WiFiState::CONNECTING;
}

/***********************************************************************
 *                      updateConnection()
 ***********************************************************************/

void WiFiManager::updateConnection()
{

    if(WiFi.status() == WL_CONNECTED)
    {
        currentState = WiFiState::CONNECTED;

        printDebug("");

        printDebug("Conexion WiFi establecida.");

        printConnectionInfo();

        return;
    }

    if(millis() - connectionStartTime >= WIFI_TIMEOUT)
    {
        printDebug("");

        printDebug("Timeout de conexion.");

        currentState = WiFiState::CONNECTION_FAILED;

        lastReconnectAttempt = millis();
    }

}

/***********************************************************************
 *                      checkConnection()
 ***********************************************************************/

void WiFiManager::checkConnection()
{

    if(WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    printDebug("");

    printDebug("Se perdio la conexion WiFi.");

    disconnect();

    lastReconnectAttempt = millis();

    currentState = WiFiState::RECONNECTING;

}

/***********************************************************************
 *                      disconnect()
 ***********************************************************************/

void WiFiManager::disconnect()
{
    WiFi.disconnect();
}

/***********************************************************************
 *                      isConnected()
 ***********************************************************************/

bool WiFiManager::isConnected() const
{
    return (WiFi.status() == WL_CONNECTED);
}

/***********************************************************************
 *                      getState()
 ***********************************************************************/

WiFiState WiFiManager::getState() const
{
    return currentState;
}

/***********************************************************************
 *                      localIP()
 ***********************************************************************/

IPAddress WiFiManager::localIP() const
{
    return WiFi.localIP();
}

/***********************************************************************
 *                      RSSI()
 ***********************************************************************/

long WiFiManager::RSSI() const
{
    return WiFi.RSSI();
}

/***********************************************************************
 *              getConnectionAttempts()
 ***********************************************************************/

uint32_t WiFiManager::getConnectionAttempts() const
{
    return connectionAttempt;
}

/***********************************************************************
 *              printConnectionInfo()
 ***********************************************************************/

void WiFiManager::printConnectionInfo() const
{

    if(!DEBUG_SERIAL)
    {
        return;
    }

    Serial.println();

    Serial.println("========= INFORMACION WIFI =========");

    Serial.print("SSID : ");

    Serial.println(WiFi.SSID());

    Serial.print("IP   : ");

    Serial.println(WiFi.localIP());

    Serial.print("RSSI : ");

    Serial.print(WiFi.RSSI());

    Serial.println(" dBm");

    Serial.print("Intentos : ");

    Serial.println(connectionAttempt);

    Serial.println("===================================");

    Serial.println();

}

/***********************************************************************
 *                  printDebug()
 ***********************************************************************/

void WiFiManager::printDebug(const String& message) const
{

    if(DEBUG_SERIAL)
    {
        Serial.println(message);
    }

}