#include <Arduino.h>
#include <DHT.h>
#include "FspTimer.h"
#include <WiFiS3.h>
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "Topics.h"
#include "MQTTPayLoads.h"


//Definimos el pin y el tipo de sensor
#define DHTPIN1 A1 // Pin donde está conectado
#define DHTPIN2 A2 
#define DHTTYPE DHT11 // Tipo de sensor

//Definimos el pin para el control de humedad
const int PIN_Humidificador = 5;
//Pines para el control de la bombilla
const int zero_cross = 3;
const int disparador = 4;
const int ventilador = 8;


DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

//Pines del invernadero
const int pin_sensorNivelagua = A0; // Pin analógico para el sensor de nivel de agua

// Instancia global del temporizador 1
FspTimer temporizador1;

WiFiManager wifiManager;
MQTTManager mqttManager(wifiManager);

bool bombillaActiva = false;
bool humidificadorActivo = false;
bool ventiladorActivo = false;
bool estadoInicialPublicado = false;

//Variables para guardar las temperaturas y humedades
volatile float temperatura1 = 20;
volatile float humedad1 = 0;
volatile float temperatura2 = 21;
volatile float humedad2 = 0;
volatile float temperaturaPromedio = 0;
volatile float humedadPromedio = 0;

//Variable para almacenar el nivel del agua
volatile float nivel_agua = 0;

//Bandera para detener el timer2
volatile bool disparar = false;

//Variables para usar el serial de momento
String mensajeRecibido = "";     // Aquí se guardará el texto final (sin el '_')
String bufferTemporal = "";      // Va acumulando los caracteres que van llegando

//Variable para gurdar la frecuencia de disparo
float frecuencia_disparo = 120*8; // Frecuencia de disparo en Hz

//Variables para el control PID del piezoeléctrico

// Duty cycle (0 - 100 %)
static unsigned long inicioPeriodo = 0;
volatile float duty = 0.4; // Valor inicial del duty cycle (40%)
int setpoint = 75; // Valor de referencia para la humedad relativa
unsigned long periodo_ms = 1000; // Periodo de la señal PWM en milisegundos

//Posibles estados que puede tener la maquina de estados
typedef enum {
  IDLE,
  enviarDatos,
  medirTemperatura,
  tempAlta,
  medirRH_Talta,
  RHbaja_Talta,
  RHalta_Talta,
  medirRH_Tbaja,
  RHbaja_Tbaja,
  RHalta_Tbaja,
  RHmedia_Tbaja,
} Estado;

Estado estadoActual = medirTemperatura; // Estado inicial de la máquina de estados

typedef enum {
  automatico,
  manual
} Modo;
Modo modoControl = automatico;


//Funciones para el manejo de la maquina de estados
void funcion_enviarTemperatura();
void funcion_enviarNivelAgua();
void configurarTimer(float frecuenciaHz);
void funcionInterrupcion(timer_callback_args_t *args);

void funcionInterpretarMensaje(const String& mensaje);
void serialEvent();
void funcionPara_disparar ();
void calcularDutyCycle();
void PWM_Humidificador();
void manejarMensajeMQTT(const char* topic, const char* payload);
void publicarEstadoActuadores();
void publicarEstadoControl();

void maquinaDeEstados();
void leerSensores();

void setup() {
    Serial.begin(115200);
    dht1.begin();
    dht2.begin();
    configurarTimer(0.5f); // Configuramos el temporizador para que interrumpa cada 2 s
    pinMode(PIN_Humidificador, OUTPUT);
    digitalWrite(PIN_Humidificador,LOW);
    pinMode(zero_cross, INPUT); // Configuramos el pin del cruce por cero como entrada
    //attachInterrupt(digitalPinToInterrupt(zero_cross), funcionPara_disparar, RISING); // Configuramos la interrupción para el cruce por cero
    pinMode(disparador,OUTPUT);
    digitalWrite(ventilador,HIGH);
    digitalWrite(disparador,LOW);
    digitalWrite(PIN_Humidificador,LOW);
    inicioPeriodo = millis();

    wifiManager.begin();
    mqttManager.begin();
    mqttManager.setMessageHandler(manejarMensajeMQTT);

}

void loop() {
  wifiManager.loop();
  mqttManager.loop();

  if (mqttManager.isConnected() && !estadoInicialPublicado) {
    publicarEstadoActuadores();
    publicarEstadoControl();
    estadoInicialPublicado = true;
  }

  maquinaDeEstados();
}

void maquinaDeEstados() {
  switch (modoControl) {
    case automatico:
      // En modo automático, ejecutamos la máquina de estados normal
        switch (estadoActual) {
          case IDLE:
            break;

          case enviarDatos:
            funcion_enviarTemperatura();
            funcion_enviarNivelAgua();
            //Condicional para generar el estado de alarma
            if (nivel_agua < 7){
              Serial.println("A_");
            }
            if (modoControl == automatico) {
              estadoActual = medirTemperatura;
            }
            else {
              estadoActual = IDLE;
            }
            break;

          case medirTemperatura:
              temperatura1 = dht1.readTemperature();
              temperatura2 = dht2.readTemperature();
              temperaturaPromedio = (temperatura1 + temperatura2) / 2.0f;

              if (temperaturaPromedio > 40){
                estadoActual = tempAlta;
              }
              else{
                estadoActual = medirRH_Tbaja;
              }
              break;

          case tempAlta:
              // Si la temperatura es alta, encendemos el ventilador
              detachInterrupt(digitalPinToInterrupt(zero_cross)); // Deshabilitamos la interrupción del cruce por cero
              bombillaActiva = false;
              digitalWrite(disparador,LOW);
              digitalWrite(ventilador, LOW); // Encender el ventilador
              ventiladorActivo = true;
              estadoActual = medirRH_Talta;
              break;

          
          case medirRH_Talta:
              humedad1 = dht1.readHumidity();
              humedad2 = dht2.readHumidity();
              humedadPromedio = (humedad1 + humedad2) / 2.0f;

              if (humedadPromedio < 70){
                estadoActual = RHbaja_Talta;
              }
              else{
                estadoActual = RHalta_Talta;
              }
              break;

          case RHbaja_Talta:
              humidificadorActivo = true;
              calcularDutyCycle(); // Calculamos el duty cycle para el humidificador
              PWM_Humidificador(); // Controlamos el humidificador con el duty cycle calculado
              estadoActual = medirTemperatura; // Volvemos a medir la humedad
              break;

          case RHalta_Talta:
              humidificadorActivo = false;
              digitalWrite(PIN_Humidificador,LOW); // Apagamos el humidificador
              estadoActual = medirTemperatura; // Volvemos a medir la humedad
              break;

          case medirRH_Tbaja:
              humedad1 = dht1.readHumidity();
              humedad2 = dht2.readHumidity();
              humedadPromedio = (humedad1 + humedad2) / 2.0f;

              if (humedadPromedio < 70){
                estadoActual = RHbaja_Tbaja;
              }
              else if (humedadPromedio >= 70 && humedadPromedio <= 80){
                estadoActual = RHmedia_Tbaja;
              }
              else{
                estadoActual = RHalta_Tbaja;
              }
              break;
          
          case RHbaja_Tbaja:
              humidificadorActivo = true;
              calcularDutyCycle(); // Calculamos el duty cycle para el humidificador
              PWM_Humidificador();// Controlamos el humidificador con el duty cycle calculado
              digitalWrite(disparador,LOW);
              detachInterrupt(digitalPinToInterrupt(zero_cross)); // Deshabilitamos la interrupción del cruce por cero
              bombillaActiva = false;
              digitalWrite(ventilador, HIGH); // Encender el ventilador
              ventiladorActivo = false;
              estadoActual = medirTemperatura; // Volvemos a medir la humedad
              break;

          case RHmedia_Tbaja:
              humidificadorActivo = false;
              digitalWrite(disparador,LOW);
              detachInterrupt(digitalPinToInterrupt(zero_cross)); // Deshabilitamos la interrupción del cruce por cero
              bombillaActiva = false;
              digitalWrite(ventilador, LOW); // Encender el ventilador
              ventiladorActivo = true;
              digitalWrite(PIN_Humidificador,LOW); // Apagamos el humidificador
              estadoActual = medirTemperatura; // Volvemos a medir la humedad
              break;

          case RHalta_Tbaja:
              attachInterrupt(digitalPinToInterrupt(zero_cross), funcionPara_disparar, RISING); // Habilitamos la interrupción del cruce por cero
              bombillaActiva = true;
              digitalWrite(ventilador, HIGH); // Apagamos el ventilador
              ventiladorActivo = false;
              digitalWrite(PIN_Humidificador,LOW); // Apagamos el humidificador
              humidificadorActivo = false;
              estadoActual = medirTemperatura; // Volvemos a medir la humedad
              break;

          default:
            // Si por alguna razón el estado es inválido, volvemos a IDLE
            estadoActual = IDLE;
            break;
          }
      break;

    case manual:
      // En modo manual, no hacemos nada en la máquina de estados
      break;;

    default:
      // Si por alguna razón el modo de control es inválido, también salimos
      break;
  }

}

void configurarTimer(float frecuenciaHz) {
    uint8_t tipo_timer = 0;
    int canal_timer = 0;

    // 1. Buscar un canal de temporizador AGT (Asynchronous General-Purpose Timer) disponible
    if (!FspTimer::get_available_timer(tipo_timer, canal_timer)) {
        Serial.println("Error: No hay temporizadores disponibles.");
        return;
    }

    // 2. Configurar las propiedades del temporizador
    // Usamos el modo PERIODIC y el temporizador AGT seleccionado
    temporizador1.begin(TIMER_MODE_PERIODIC, tipo_timer, canal_timer, frecuenciaHz, 50.0f, funcionInterrupcion, nullptr);

    // 3. Habilitar la interrupción en el controlador de interrupciones (NVIC)
    temporizador1.setup_overflow_irq();

    // 4. Abrir e iniciar el temporizador
    temporizador1.open();
    temporizador1.start();

    Serial.print("Temporizador configurado en el canal: ");
    Serial.println(canal_timer);
}


void funcionInterrupcion(timer_callback_args_t *args) {
    estadoActual = enviarDatos; // Cambiamos al estado de enviar datos
   
}

void leerSensores() {
    temperatura1 = dht1.readTemperature();
    temperatura2 = dht2.readTemperature();
    temperaturaPromedio = (temperatura1 + temperatura2) / 2.0f;

    humedad1 = dht1.readHumidity();
    humedad2 = dht2.readHumidity();
    humedadPromedio = (humedad1 + humedad2) / 2.0f;
}

void funcion_enviarTemperatura() {
    Serial.print("T_");
    Serial.print (temperatura1);
    Serial.print ("_");
    Serial.print (temperatura2);
    Serial.print ("_");
    Serial.print (temperaturaPromedio);

    mqttManager.publish(TOPIC_SENSOR_TEMPERATURA,
                         String(temperaturaPromedio, 2).c_str());
    mqttManager.publish(TOPIC_SENSOR_HUMEDAD,
                         String(humedadPromedio, 2).c_str());
    publicarEstadoActuadores();
    publicarEstadoControl();
}


void funcion_enviarNivelAgua (){
  float lecturaADC = analogRead(pin_sensorNivelagua); // Leer el valor del sensor de nivel de agua

  // Convertir la lectura ADC a voltaje (0-5V)
  float voltaje = lecturaADC *  (5/ 1023.0); 
  if (voltaje <= 2.9) {
    nivel_agua = 5*voltaje;
  }
  else if (voltaje > 2.9 && voltaje <= 3.1 ){
    nivel_agua = 7*voltaje;
  }
  else if (voltaje > 3.1 && voltaje<= 3.2){
    nivel_agua = 10*voltaje;
  }
  else{
    nivel_agua = 14.2*voltaje;
  }

  mqttManager.publish(TOPIC_SENSOR_NIVEL_AGUA,
                      String(int(nivel_agua)).c_str());

  if (nivel_agua < 7) {
    mqttManager.publish(TOPIC_SISTEMA_ALARMA,
                        ALARM_TANQUE_VACIO);
  }
}

void serialEvent() {
  // Mientras haya bytes en el búfer de hardware, los procesamos de inmediato
  while (Serial.available()) {
    char caracterEntrante = (char)Serial.read();
    
    // Si encontramos el carácter indicador '_'
    if (caracterEntrante == '_') {
      mensajeRecibido = bufferTemporal; // Guardamos TODO el texto acumulado HASTA AHORA
      bufferTemporal = "";              // Limpiamos el buffer para el siguiente mensaje
      funcionInterpretarMensaje(mensajeRecibido); // Cambiamos al estado de interpretación de comando
    } 
    // Si no es el '_', y tampoco son caracteres basura de control (como el salto de línea)
    else if (caracterEntrante != '\n' && caracterEntrante != '\r') {
      bufferTemporal += caracterEntrante; // Seguimos acumulando el texto
    }
  }
}

void funcionInterpretarMensaje(const String& mensaje)
{
  if (mensaje == "A" || mensaje.equalsIgnoreCase(PAYLOAD_AUTO)) {
    modoControl = automatico;
    publicarEstadoControl();
  }
  else if (mensaje == "M" || mensaje.equalsIgnoreCase(PAYLOAD_MANUAL)) {
    modoControl = manual;
    publicarEstadoControl();
  }
  else if(mensaje.equalsIgnoreCase("V1") || mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
    if (modoControl == manual) {
      digitalWrite(ventilador,LOW);
      ventiladorActivo = true;
      publicarEstadoActuadores();
    }
  }
 
  else if(mensaje.equalsIgnoreCase("V0") || mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
    if (modoControl == manual) {
      digitalWrite(ventilador,HIGH);
      ventiladorActivo = false;
      publicarEstadoActuadores();
    }
  }
  else if (mensaje.equalsIgnoreCase("B1") || mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
    if (modoControl == manual) {
      attachInterrupt(digitalPinToInterrupt(zero_cross), funcionPara_disparar, RISING);
      bombillaActiva = true;
      publicarEstadoActuadores();
    }
  }
  else if (mensaje.equalsIgnoreCase("B0") || mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
    if (modoControl == manual) {
      detachInterrupt(digitalPinToInterrupt(zero_cross));
      bombillaActiva = false;
      digitalWrite(disparador,LOW);
      publicarEstadoActuadores();
    }
  }
  else {
    // Comando no reconocido, volver a IDLE
    estadoActual = IDLE;
  }
}

void funcionPara_disparar (){
    //disparar = true;
    digitalWrite(disparador,LOW);
    delay(6);
    digitalWrite(disparador,HIGH);
}

void manejarMensajeMQTT(const char* topic, const char* payload)
{
    String mensaje = String(payload);

    if (String(topic) == TOPIC_CFG_MODO) {
        funcionInterpretarMensaje(mensaje);
    }
    else if (String(topic) == TOPIC_CFG_SETPOINT) {
        setpoint = mensaje.toInt();
        publicarEstadoControl();
    }
    else if (String(topic) == TOPIC_CMD_BOMBILLA) {
        if (mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
            funcionInterpretarMensaje("B1");
        }
        else if (mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
            funcionInterpretarMensaje("B0");
        }
    }
    else if (String(topic) == TOPIC_CMD_VENTILADOR) {
        if (mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
            funcionInterpretarMensaje("V1");
        }
        else if (mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
            funcionInterpretarMensaje("V0");
        }
    }
    else if (String(topic) == TOPIC_CMD_HUMIDIFICADOR) {
        if (mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
            humidificadorActivo = true;
            digitalWrite(PIN_Humidificador, HIGH);
            publicarEstadoActuadores();
        }
        else if (mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
            humidificadorActivo = false;
            digitalWrite(PIN_Humidificador, LOW);
            publicarEstadoActuadores();
        }
    }
}

void publicarEstadoActuadores()
{
    String payload = "VENTILADOR=";
    payload += ventiladorActivo ? PAYLOAD_ON : PAYLOAD_OFF;
    payload += ";BOMBILLA=";
    payload += bombillaActiva ? PAYLOAD_ON : PAYLOAD_OFF;
    payload += ";HUMIDIFICADOR=";
    payload += humidificadorActivo ? PAYLOAD_ON : PAYLOAD_OFF;

    mqttManager.publish(TOPIC_ESTADO_ACTUADORES, payload.c_str());
    mqttManager.publish(TOPIC_ESTADO_VENTILADOR, ventiladorActivo ? PAYLOAD_ON : PAYLOAD_OFF);
    mqttManager.publish(TOPIC_ESTADO_BOMBILLA, bombillaActiva ? PAYLOAD_ON : PAYLOAD_OFF);
    mqttManager.publish(TOPIC_ESTADO_HUMIDIFICADOR, humidificadorActivo ? PAYLOAD_ON : PAYLOAD_OFF);
}

void publicarEstadoControl()
{
    mqttManager.publish(TOPIC_ESTADO_MODO,
                        (modoControl == automatico) ? PAYLOAD_AUTO : PAYLOAD_MANUAL);
    mqttManager.publish(TOPIC_ESTADO_SETPOINT,
                        String(setpoint).c_str());
}

void calcularDutyCycle() {
    // Constantes del PID
    const float Kp = 1.0; // Ganancia proporcional
    const float Ki = 0.1; // Ganancia integral
    const float Kd = 0.05; // Ganancia derivativa

    static float errorAnterior = 0;
    static float integral = 0;

    // Calcular el error
    float error = setpoint - humedadPromedio;

    // Calcular la integral y la derivada
    integral += error;
    float derivada = error - errorAnterior;

    // Calcular el output del PID
    duty = Kp * error + Ki * integral + Kd * derivada;

    duty = duty/100.0; // Convertir a porcentaje

    // Guardar el error actual para la próxima iteración
    errorAnterior = error;

    // Asegurarse de que el duty cycle esté entre 0 y 1
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;
}

void PWM_Humidificador(){
    unsigned long ahora = millis();

    unsigned long tiempo = ahora - inicioPeriodo;

    if(tiempo >= periodo_ms)
    {
        inicioPeriodo += periodo_ms;

        tiempo = ahora - inicioPeriodo;
    }

    unsigned long tiempoON = periodo_ms * duty;

    digitalWrite(PIN_Humidificador, tiempo < tiempoON);
}


