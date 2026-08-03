#ifndef MQTT_PAYLOADS_H
#define MQTT_PAYLOADS_H

/***********************************************************************
 *
 *                      MQTT Payloads
 *
 *  Proyecto:
 *      Invernadero Inteligente IoT
 *
 *  Descripción:
 *      Define todos los payloads utilizados en la comunicación MQTT.
 *
 *      Ningún otro archivo debe escribir directamente cadenas
 *      como "ON", "OFF", "AUTO", etc.
 *
 ***********************************************************************/

#include <Arduino.h>

/***********************************************************************
 *                      ACTUADORES
 ***********************************************************************/

constexpr char PAYLOAD_ON[]  = "ON";
constexpr char PAYLOAD_OFF[] = "OFF";

/***********************************************************************
 *                  MODOS DE OPERACIÓN
 ***********************************************************************/

constexpr char PAYLOAD_AUTO[]   = "AUTO";
constexpr char PAYLOAD_MANUAL[] = "MANUAL";

/***********************************************************************
 *                  ESTADO DE CONEXIÓN
 ***********************************************************************/

constexpr char PAYLOAD_ONLINE[]  = "ONLINE";
constexpr char PAYLOAD_OFFLINE[] = "OFFLINE";

/***********************************************************************
 *                      ALARMAS
 ***********************************************************************/

constexpr char ALARM_TANQUE_VACIO[]      = "TANQUE_VACIO";

constexpr char ALARM_SENSOR_NIVEL[]      = "ERROR_SENSOR_NIVEL";

constexpr char ALARM_SENSOR_DHT[]        = "ERROR_SENSOR_DHT";

constexpr char ALARM_SENSOR_HUMEDAD[]    = "ERROR_SENSOR_HUMEDAD";

constexpr char ALARM_SENSOR_TEMPERATURA[]= "ERROR_SENSOR_TEMPERATURA";

/***********************************************************************
 *                      ERRORES MQTT
 ***********************************************************************/

constexpr char ERROR_WIFI[]      = "ERROR_WIFI";

constexpr char ERROR_MQTT[]      = "ERROR_MQTT";

constexpr char ERROR_BROKER[]    = "ERROR_BROKER";

/***********************************************************************
 *                  RESPUESTAS GENERALES
 ***********************************************************************/

constexpr char RESPONSE_OK[]    = "OK";

constexpr char RESPONSE_ERROR[] = "ERROR";

#endif