#ifndef TOPICS_H
#define TOPICS_H

/***********************************************************************
 *
 *                      Topics MQTT
 *
 *  Proyecto:
 *      Invernadero Inteligente IoT
 *
 *  Descripción:
 *      Todos los topics MQTT utilizados por el firmware
 *      y la aplicación Web.
 *
 *  IMPORTANTE:
 *      Ningún otro archivo debe escribir directamente
 *      el nombre de un topic.
 *
 ***********************************************************************/

#include <Arduino.h>

/***********************************************************************
 *                  TOPIC RAÍZ
 ***********************************************************************/

constexpr char ROOT_TOPIC[] = "invernadero";

/***********************************************************************
 *                  SENSORES
 *
 *  Publica:
 *      Arduino
 *
 *  Se suscribe:
 *      Aplicación Web
 ***********************************************************************/

constexpr char TOPIC_SENSOR_TEMPERATURA[] =
"invernadero/sensores/temperatura";

constexpr char TOPIC_SENSOR_HUMEDAD[] =
"invernadero/sensores/humedad";

constexpr char TOPIC_SENSOR_NIVEL_AGUA[] =
"invernadero/sensores/nivel_agua";

/***********************************************************************
 *                  COMANDOS
 *
 *  Publica:
 *      Aplicación Web
 *
 *  Se suscribe:
 *      Arduino
 ***********************************************************************/

constexpr char TOPIC_CMD_BOMBILLA[] =
"invernadero/comandos/bombilla";

constexpr char TOPIC_CMD_VENTILADOR[] =
"invernadero/comandos/ventilador";

constexpr char TOPIC_CMD_HUMIDIFICADOR[] =
"invernadero/comandos/humidificador";

/***********************************************************************
 *                  CONFIGURACIÓN
 *
 *  Publica:
 *      Aplicación Web
 *
 *  Se suscribe:
 *      Arduino
 ***********************************************************************/

constexpr char TOPIC_CFG_MODO[] =
"invernadero/configuracion/modo";

constexpr char TOPIC_CFG_SETPOINT[] =
"invernadero/configuracion/setpoint";

/***********************************************************************
 *                  ESTADO
 *
 *  Publica:
 *      Arduino
 *
 *  Se suscribe:
 *      Aplicación Web
 ***********************************************************************/

constexpr char TOPIC_ESTADO_ACTUADORES[] =
"invernadero/estado/actuadores";

constexpr char TOPIC_ESTADO_BOMBILLA[] =
"invernadero/estado/bombilla";

constexpr char TOPIC_ESTADO_VENTILADOR[] =
"invernadero/estado/ventilador";

constexpr char TOPIC_ESTADO_HUMIDIFICADOR[] =
"invernadero/estado/humidificador";

constexpr char TOPIC_ESTADO_MODO[] =
"invernadero/estado/modo";

constexpr char TOPIC_ESTADO_SETPOINT[] =
"invernadero/estado/setpoint";

/***********************************************************************
 *                  SISTEMA
 *
 *  Publica:
 *      Arduino
 *
 *  Se suscribe:
 *      Aplicación Web
 ***********************************************************************/

constexpr char TOPIC_SISTEMA_CONEXION[] =
"invernadero/sistema/conexion";

constexpr char TOPIC_SISTEMA_ALARMA[] =
"invernadero/sistema/alarma";

constexpr char TOPIC_SISTEMA_ERROR[] =
"invernadero/sistema/error";

#endif