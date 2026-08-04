#include <Arduino.h>
#include <DHT.h>
#include "FspTimer.h"
#include <WiFiS3.h>
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "Topics.h"
#include "MQTTPayLoads.h"

// Definimos el pin y el tipo de sensor
#define DHTPIN1 A1 // Pin donde está conectado
#define DHTPIN2 A2 
#define DHTTYPE DHT11 // Tipo de sensor

// Definimos el pin para el control de humedad
const int PIN_Humidificador = 5;
// Pines para el control de la bombilla y ventilador
const int zero_cross = 3;
const int disparador = 4;
const int ventilador = 8; // Relé Active-LOW (LOW = Encendido, HIGH = Apagado)

DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

// Pines del invernadero
const int pin_sensorNivelagua = A0; // Pin analógico para el sensor de nivel de agua

// Instancia global del temporizador 1
FspTimer temporizador1;

WiFiManager wifiManager;
MQTTManager mqttManager(wifiManager);

bool bombillaActiva = false;
bool humidificadorActivo = false;
bool ventiladorActivo = false;
bool estadoInicialPublicado = false;

// Bandera para sincronizar el temporizador con la máquina de estados (Evita Race Conditions)
volatile bool flag_enviarDatos = false;

// Variables para guardar las temperaturas y humedades
volatile float temperatura1 = 20;
volatile float humedad1 = 0;
volatile float temperatura2 = 21;
volatile float humedad2 = 0;
volatile float temperaturaPromedio = 0;
volatile float humedadPromedio = 0;

// Variable para almacenar el nivel del agua
volatile float nivel_agua = 0;

// Variables para usar el serial de momento
String mensajeRecibido = "";     // Aquí se guardará el texto final (sin el '_')
String bufferTemporal = "";      // Va acumulando los caracteres que van llegando

// Variables para el control PID del piezoeléctrico
static unsigned long inicioPeriodo = 0;
volatile float duty = 0.4; // Valor inicial del duty cycle (40%)
int setpoint = 75; // Valor de referencia para la humedad relativa
unsigned long periodo_ms = 1000; // Periodo de la señal PWM en milisegundos (1 Hz)

// Posibles estados que puede tener la maquina de estados
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

Estado estadoActual = IDLE; // Estado inicial corregido a IDLE para esperar el primer disparo del timer

typedef enum {
  automatico,
  manual
} Modo;
Modo modoControl = automatico;

// Prototipos de funciones
void funcion_enviarTemperatura();
void funcion_enviarNivelAgua();
void configurarTimer(float frecuenciaHz);
void funcionInterrupcion(timer_callback_args_t *args);
void funcionInterpretarMensaje(const String& mensaje);
void serialEvent();
void funcionPara_disparar();
void calcularDutyCycle();
void PWM_Humidificador();
void manejarMensajeMQTT(const char* topic, const char* payload);
void publicarEstadoActuadores();
void publicarEstadoControl();
void maquinaDeEstados();
void leerSensores();
void aplicarEstadoActuadores();

void setup() {
    Serial.begin(115200);
    dht1.begin();
    dht2.begin();
    configurarTimer(0.5f); // Configuramos el temporizador para que interrumpa cada 2 s
    
    pinMode(PIN_Humidificador, OUTPUT);
    digitalWrite(PIN_Humidificador, LOW);
    
    pinMode(zero_cross, INPUT); // Configuramos el pin del cruce por cero como entrada
    
    pinMode(disparador, OUTPUT);
    digitalWrite(disparador, LOW);
    
    pinMode(ventilador, OUTPUT);
    digitalWrite(ventilador, HIGH); // Apagado por defecto (Relé Active-LOW)
    aplicarEstadoActuadores();
    
    inicioPeriodo = millis();

    wifiManager.begin();
    mqttManager.begin();
    mqttManager.setMessageHandler(manejarMensajeMQTT);
}

void loop() {
  
  wifiManager.loop();
  mqttManager.loop();
  
  // Ejecutamos serialEvent explícitamente para el Arduino UNO R4
  serialEvent();

  if (mqttManager.isConnected() && !estadoInicialPublicado) {
    publicarEstadoActuadores();
    publicarEstadoControl();
    estadoInicialPublicado = true;
  }

  // Máquina de estados controlada por la bandera del temporizador
  maquinaDeEstados();
  aplicarEstadoActuadores();
  
  // El PWM debe correr continuamente, independiente de los estados
  PWM_Humidificador();
}

void maquinaDeEstados() {
  switch (modoControl) {
    case automatico:
        switch (estadoActual) {
          case IDLE:
            // Solo salimos de IDLE si el timer levantó la bandera
            if (flag_enviarDatos == true) {
                flag_enviarDatos = false;
                estadoActual = enviarDatos;
            }
            break;

          case enviarDatos:
            funcion_enviarTemperatura();
            funcion_enviarNivelAgua();
            // Condicional para generar el estado de alarma
            if (nivel_agua < 7){
              Serial.println("A_");
            }
            if (modoControl == automatico) {
              estadoActual = medirTemperatura;
            } else {
              estadoActual = IDLE;
            }
            break;

          case medirTemperatura:
              temperatura1 = dht1.readTemperature();
              temperatura2 = dht2.readTemperature();
              temperaturaPromedio = (temperatura1 + temperatura2) / 2.0f;

              if (temperaturaPromedio > 40){
                estadoActual = tempAlta;
              } else {
                estadoActual = medirRH_Tbaja;
              }
              break;

          case tempAlta:
              // Si la temperatura is alta, apagamos bombilla y encendemos ventilador
              detachInterrupt(digitalPinToInterrupt(zero_cross)); 
              bombillaActiva = false;
              digitalWrite(disparador, LOW);
              digitalWrite(ventilador, LOW); // LOW enciende el relé
              ventiladorActivo = true;
              estadoActual = medirRH_Talta;
              break;
          
          case medirRH_Talta:
              humedad1 = dht1.readHumidity();
              humedad2 = dht2.readHumidity();
              Serial.print("Humedad1: ");
              Serial.println(humedad1);
              Serial.print("Humedad2: ");
              Serial.println(humedad2);
              humedadPromedio = (humedad1 + humedad2) / 2.0f;

              if (humedadPromedio < 70){
                estadoActual = RHbaja_Talta;
              } else {
                estadoActual = RHalta_Talta;
              }
              break;

          case RHbaja_Talta:
              humidificadorActivo = true;
              calcularDutyCycle(); // Calculamos, pero el loop ejecuta el PWM
              estadoActual = IDLE; // Volvemos a reposo para no saturar el DHT11
              break;

          case RHalta_Talta:
              humidificadorActivo = false;
              // El loop apagará el humidificador automáticamente
              estadoActual = IDLE;
              break;

          case medirRH_Tbaja:
              humedad1 = dht1.readHumidity();
              humedad2 = dht2.readHumidity();
              humedadPromedio = (humedad1 + humedad2) / 2.0f;

              if (humedadPromedio < 70){
                estadoActual = RHbaja_Tbaja;
              } else if (humedadPromedio >= 70 && humedadPromedio <= 80){
                estadoActual = RHmedia_Tbaja;
              } else {
                estadoActual = RHalta_Tbaja;
              }
              break;
          
          case RHbaja_Tbaja:
              humidificadorActivo = true;
              calcularDutyCycle(); 
              
              digitalWrite(disparador, LOW);
              detachInterrupt(digitalPinToInterrupt(zero_cross)); 
              bombillaActiva = false;
              
              digitalWrite(ventilador, HIGH); // Apagamos el ventilador (HIGH)
              ventiladorActivo = false;
              
              estadoActual = IDLE;
              break;

          case RHmedia_Tbaja:
              humidificadorActivo = false;
              
              digitalWrite(disparador, LOW);
              detachInterrupt(digitalPinToInterrupt(zero_cross)); 
              bombillaActiva = false;
              
              digitalWrite(ventilador, LOW); // Encendemos el ventilador (LOW)
              ventiladorActivo = true;
              
              estadoActual = IDLE;
              break;

          case RHalta_Tbaja:
              attachInterrupt(digitalPinToInterrupt(zero_cross), funcionPara_disparar, RISING); 
              bombillaActiva = true;
              
              digitalWrite(ventilador, HIGH); // Apagamos el ventilador (HIGH)
              ventiladorActivo = false;
              
              humidificadorActivo = false;
              
              estadoActual = IDLE;
              break;

          default:
            estadoActual = IDLE;
            break;
          }
      break;

    case manual:
          switch (estadoActual) {
          case IDLE:
            // Solo salimos de IDLE si el timer levantó la bandera
            if (flag_enviarDatos == true) {
                flag_enviarDatos = false;
                estadoActual = enviarDatos;
            }
            break;

          case enviarDatos:
            funcion_enviarTemperatura();
            funcion_enviarNivelAgua();
            // Condicional para generar el estado de alarma
            if (nivel_agua < 7){
              Serial.println("A_");
            }
            if (modoControl == automatico) {
              estadoActual = medirTemperatura;
            } else {
              estadoActual = medirTemperatura; // En modo manual, pasamos a medir sensores para actualizar el PID y datos
            }
            break;

          case medirTemperatura:
              temperatura1 = dht1.readTemperature();
              temperatura2 = dht2.readTemperature();
              temperaturaPromedio = (temperatura1 + temperatura2) / 2.0f;
              humedad1 = dht1.readHumidity();
              humedad2 = dht2.readHumidity();
              humedadPromedio = (humedad1 + humedad2) / 2.0f;

              // En modo manual, si el humidificador está activo, ejecutamos el PID con el setpoint recibido
              if (humidificadorActivo) {
                  calcularDutyCycle();
              }
              
              estadoActual = IDLE; 
              break;
          
          default:
            estadoActual = IDLE;
            break;
          }
          break;

    default:
      break;
  }
}

void configurarTimer(float frecuenciaHz) {
    uint8_t tipo_timer = 0;
    int canal_timer = 0;

    if (!FspTimer::get_available_timer(tipo_timer, canal_timer)) {
        Serial.println("Error: No hay temporizadores disponibles.");
        return;
    }

    temporizador1.begin(TIMER_MODE_PERIODIC, tipo_timer, canal_timer, frecuenciaHz, 50.0f, funcionInterrupcion, nullptr);
    temporizador1.setup_overflow_irq();
    temporizador1.open();
    temporizador1.start();

    Serial.print("Temporizador configurado en el canal: ");
    Serial.println(canal_timer);
}

void funcionInterrupcion(timer_callback_args_t *args) {
    flag_enviarDatos = true; // Solo levantamos bandera (evita Condition Race)
}

void leerSensores() {
    float lecturaTemperatura1 = dht1.readTemperature();
    float lecturaTemperatura2 = dht2.readTemperature();
    float lecturaHumedad1 = dht1.readHumidity();
    float lecturaHumedad2 = dht2.readHumidity();

    if (!isnan(lecturaTemperatura1)) {
        temperatura1 = lecturaTemperatura1;
    }
    if (!isnan(lecturaTemperatura2)) {
        temperatura2 = lecturaTemperatura2;
    }
    if (!isnan(lecturaHumedad1)) {
        humedad1 = lecturaHumedad1;
    }
    if (!isnan(lecturaHumedad2)) {
        humedad2 = lecturaHumedad2;
    }

    temperaturaPromedio = (temperatura1 + temperatura2) / 2.0f;
    humedadPromedio = (humedad1 + humedad2) / 2.0f;
}

void aplicarEstadoActuadores() {
    static bool interruptBombillaRegistrado = false;

    if (bombillaActiva) {
        if (!interruptBombillaRegistrado) {
            attachInterrupt(digitalPinToInterrupt(zero_cross), funcionPara_disparar, RISING);
            interruptBombillaRegistrado = true;
        }
        digitalWrite(disparador, LOW);
    } else {
        if (interruptBombillaRegistrado) {
            detachInterrupt(digitalPinToInterrupt(zero_cross));
            interruptBombillaRegistrado = false;
        }
        digitalWrite(disparador, LOW);
    }

    if (ventiladorActivo) {
        digitalWrite(ventilador, LOW);
    } else {
        digitalWrite(ventilador, HIGH);
    }
}

void funcion_enviarTemperatura() {
    leerSensores();
    Serial.print("T_");
    Serial.print (temperatura1);
    Serial.print ("_");
    Serial.print (temperatura2);
    Serial.print ("_");
    Serial.print (temperaturaPromedio);
    Serial.println(); 

    mqttManager.publish(TOPIC_SENSOR_TEMPERATURA,
                         String(temperaturaPromedio, 2).c_str());
    mqttManager.publish(TOPIC_SENSOR_HUMEDAD,
                         String(humedadPromedio, 2).c_str());
    publicarEstadoActuadores();
    publicarEstadoControl();
}

void funcion_enviarNivelAgua (){
  float lecturaADC = analogRead(pin_sensorNivelagua); 
  float voltaje = lecturaADC * (5.0 / 1023.0); 
  
  if (voltaje <= 2.9) {
    nivel_agua = 5 * voltaje;
  }
  else if (voltaje > 2.9 && voltaje <= 3.1 ){
    nivel_agua = 7 * voltaje;
  }
  else if (voltaje > 3.1 && voltaje <= 3.2){
    nivel_agua = 10 * voltaje;
  }
  else{
    nivel_agua = 14.2 * voltaje;
  }

  mqttManager.publish(TOPIC_SENSOR_NIVEL_AGUA, String(int(nivel_agua)).c_str());

  if (nivel_agua < 7) {
    mqttManager.publish(TOPIC_SISTEMA_ALARMA, ALARM_TANQUE_VACIO);
  }
}

void serialEvent() {
  while (Serial.available()) {
    char caracterEntrante = (char)Serial.read();
    
    if (caracterEntrante == '_') {
      mensajeRecibido = bufferTemporal; 
      bufferTemporal = "";              
      funcionInterpretarMensaje(mensajeRecibido); 
    } 
    else if (caracterEntrante != '\n' && caracterEntrante != '\r') {
      bufferTemporal += caracterEntrante; 
    }
  }
}

void funcionInterpretarMensaje(const String& mensaje) {
  if (mensaje == "A" || mensaje.equalsIgnoreCase(PAYLOAD_AUTO)) {
    modoControl = automatico;
    publicarEstadoControl();
  }
  else if (mensaje == "M" || mensaje.equalsIgnoreCase(PAYLOAD_MANUAL)) {
    modoControl = manual;
    publicarEstadoControl();
  }
  else if(mensaje.equalsIgnoreCase("V1") || mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
    ventiladorActivo = true;
    aplicarEstadoActuadores();
    publicarEstadoActuadores();
  }
  else if(mensaje.equalsIgnoreCase("V0") || mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
    ventiladorActivo = false;
    aplicarEstadoActuadores();
    publicarEstadoActuadores();
  }
  else if (mensaje.equalsIgnoreCase("B1")) {
    bombillaActiva = true;
    aplicarEstadoActuadores();
    publicarEstadoActuadores();
  }
  else if (mensaje.equalsIgnoreCase("B0")) {
    bombillaActiva = false;
    aplicarEstadoActuadores();
    publicarEstadoActuadores();
  }
}

void funcionPara_disparar (){
    digitalWrite(disparador, LOW);
    delay(6); // Retardo para el Triac 
    digitalWrite(disparador, HIGH);
}

void manejarMensajeMQTT(const char* topic, const char* payload) {
    String mensaje = String(payload);
    String topicStr = String(topic);

    if (topicStr == TOPIC_CFG_MODO || topicStr.endsWith("/configuracion/modo")) {
        funcionInterpretarMensaje(mensaje);
    }
    else if (topicStr == TOPIC_CFG_SETPOINT || topicStr.endsWith("/configuracion/setpoint")) {
        setpoint = mensaje.toInt();
        publicarEstadoControl();
    }
    else if (topicStr == TOPIC_CMD_BOMBILLA || topicStr.endsWith("/comandos/bombilla")) {
        if (mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
            funcionInterpretarMensaje("B1");
        }
        else if (mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
            funcionInterpretarMensaje("B0");
        }
    }
    else if (topicStr == TOPIC_CMD_VENTILADOR || topicStr.endsWith("/comandos/ventilador")) {
        if (mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
            funcionInterpretarMensaje("V1");
        }
        else if (mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
            funcionInterpretarMensaje("V0");
        }
    }
    else if (topicStr == TOPIC_CMD_HUMIDIFICADOR || topicStr.endsWith("/comandos/humidificador")) {
        if (mensaje.equalsIgnoreCase(PAYLOAD_ON)) {
            humidificadorActivo = true;
            aplicarEstadoActuadores();
            publicarEstadoActuadores();
        }
        else if (mensaje.equalsIgnoreCase(PAYLOAD_OFF)) {
            humidificadorActivo = false;
            aplicarEstadoActuadores();
            publicarEstadoActuadores();
        }
    }
}

void publicarEstadoActuadores() {
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

void publicarEstadoControl() {
    mqttManager.publish(TOPIC_ESTADO_MODO,
                        (modoControl == automatico) ? PAYLOAD_AUTO : PAYLOAD_MANUAL);
    mqttManager.publish(TOPIC_ESTADO_SETPOINT,
                        String(setpoint).c_str());
}

void calcularDutyCycle() {
    const float Kp = 1.0; 
    const float Ki = 0.1; 
    const float Kd = 0.05; 

    static float errorAnterior = 0;
    static float integral = 0;

    float error = setpoint - humedadPromedio;
    integral += error;

    // --- ANTI-WINDUP SENCILLO ---
    if (integral > 1000.0) {
        integral = 1000.0;
    } else if (integral < 0.0) {
        integral = 0.0;
    }

    float derivada = error - errorAnterior;
    float salidaPID = (Kp * error) + (Ki * integral) + (Kd * derivada);

    duty = salidaPID / 100.0; 

    // --- LÍMITE AL DUTY CYCLE ---
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    errorAnterior = error;
    
    Serial.print("Duty cycle calculado: ");
    Serial.println(duty);
}

void PWM_Humidificador() {
    if (!humidificadorActivo) {
        digitalWrite(PIN_Humidificador, HIGH); // Apagamos el humidificador
        return; 
    }

    unsigned long tiempo_actual = millis();
    unsigned long tiempo = tiempo_actual - inicioPeriodo;
    
    if(tiempo >= periodo_ms) {
        inicioPeriodo = tiempo_actual; 
        tiempo = 0;
    }
    
    unsigned long tiempoON = periodo_ms * (1-duty);
    digitalWrite(PIN_Humidificador, tiempo < tiempoON);
}