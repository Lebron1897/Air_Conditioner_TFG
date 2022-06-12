#include <Time.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Code_Remotes.h"
#include "Metodos.h"

#define ENCENDIDO 0
#define APAGADO 1

/*EspMQTTClient client(
  "WifiSSID",
  "WifiPassword",
  "broker.hivemq.com",  // MQTT Broker server ip
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed
  "Emisor_IR",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
  );*/

//Declaracion de variables para MQTT
const char* mqtt_server = "broker.hivemq.com"; //TFG/ConectarAA
const int mqttPort = 1883;
const char* ssid = "";
const char* password = "";
const char* mqttUser = "";
const char* mqttPassword = "";

WiFiClient espClient;
PubSubClient client(espClient);

const uint16_t PinIR = 4;
uint16_t FREQ = 38;

//Declaracion de variables globales
time_t fecha_actual;
uint16_t code_temp_PANASONIC[TAM_MES_PAN];
uint16_t code_apaga_PANASONIC[TAM_MES_PAN];
uint16_t code_temp_FUJITSU[TAM_MES_ENC_FUJ];
uint16_t code_apaga_FUJITSU[TAM_MES_APA_FUJ];
uint16_t code_temp_MC[TAM_MES_MC];
uint16_t code_apaga_MC[TAM_MES_MC];
unsigned int tiempo_sens_temp_actual = 0;
unsigned int tiempo_sens_temp_inicio = 0;
unsigned int tiempo_sens_mov_actual = 0;
unsigned int tiempo_sens_mov_inicio = 0;
unsigned int tiempo_sens_prox_actual = 0;
unsigned int tiempo_sens_prox_inicio = 0;
String nombre_disp = "";
int salta_temporizador = 0;
int marca_aire = -1;
int code_temp = -1;
int code_apagado = -1;
int estado_encendido = -1;
int estado_sensores = -1;
int temp_ejecucion = -1;
int res = -1;
byte *payload;
StaticJsonDocument <200> doc;
IRsend irsend(PinIR);  // Set the GPIO to be used to sending the message.

//Creacion de las estructuras para qt, el broker y el IR
message_IR data_IR;

//Creacion de las estructuras de datos para las ESP32 con los sensores de temperatura y de proximidad
message_temp myData_temp;
message_prox myData_prox;

//Creacion de las estructuras para almacenar los datos leidos de las otras placas
message_temp board_temp;
message_prox board_prox;

//Declaracion de las variables del broker
MESSAGE_ENCENDIDO_MANUAL_PARAMETROS message_enc_man;
MESSAGE_APAGADO_MANUAL_PARAMETROS message_apa_man;
MESSAGE_CAMBIO_TEMPERATURA_PARAMETROS message_cam_temp;
MESSAGE_ENCENDIDO_PROGRAMADO_PARAMETROS message_enc_prog;
MESSAGE_APAGADO_PROGRAMADO_PARAMETROS message_apa_prog;

void setup_cliente() {
  irsend.begin();
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback_json);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32 Client", mqttUser, mqttPassword)) {
      Serial.println("connected");

      client.subscribe("Marca_Aire");
      client.subscribe("Encendido");
      client.subscribe("Apagado");
      client.subscribe("Cambio_Temperatura");

      client.subscribe("Encendido_Programado_Dia");
      client.subscribe("Encendido_Programado_Mes");
      client.subscribe("Encendido_Programado_Ano");
      client.subscribe("Encendido_Programado_Hora");
      client.subscribe("Encendido_Programado_Min");
      client.subscribe("Encendido_Programado_Temp");
      client.subscribe("Encendido_Programado_Ejec");

      client.subscribe("Apagado_Programado_Dia");
      client.subscribe("Apagado_Programado_Mes");
      client.subscribe("Apagado_Programado_Ano");
      client.subscribe("Apagado_Programado_Hora");
      client.subscribe("Apagado_Programado_Min");
      client.subscribe("Apagado_Programado_Ejec");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);

  memcpy(&myData_temp, incomingData, sizeof(myData_temp));
  memcpy(&myData_prox, incomingData, sizeof(myData_prox));
  Realizar_Copia_Mensaje_Temp();
  Realizar_Copia_Mensaje_Prox();

  if (myData_temp.id == 1) {
    Serial.printf("Board ID %u: %u bytes\n", myData_temp.id, len);
    // Update the structures with the new incoming data
    Serial.printf("Temperature: %dºC \n", myData_temp.temp);
    Serial.printf("Humidity: %d \n", myData_temp.hum);
    if (myData_temp.mov == HIGH) {
      Serial.println("Movimiento detectado!");
    }
    else {
      Serial.println("No se ha detectado movimiento!");
    }
    Serial.println();
  } else if (myData_prox.id == 2) {
    Serial.printf("Board ID %u: %u bytes\n", myData_prox.id, len);
    if (myData_prox.sens_prox == HIGH) {
      Serial.println("Ventana abierta");
    } else {
      Serial.println("Ventana cerrada");
    }
    Serial.println();
  }
}

void callback_json(char* topic, byte* payload, unsigned int length) {
  char nombre_payload[length + 1];
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  Lectura_Valores_Broker();
  while (res == -1) {
    Realizo_Accion_Aire();
  }
}

void Actualiza_Fecha_Hora() {
  fecha_actual = now();
}

void Actualiza_Timer() {
  tiempo_sens_temp_actual = millis();
  tiempo_sens_mov_actual = millis();
  tiempo_sens_prox_actual = millis();
}

void Reinicia_Timer() {
  tiempo_sens_temp_inicio = tiempo_sens_temp_actual;
  tiempo_sens_mov_inicio = tiempo_sens_mov_actual;
  tiempo_sens_prox_inicio = tiempo_sens_prox_actual;
}

void Comprueba_Timer() {
  if (tiempo_sens_temp_actual >= (tiempo_sens_temp_inicio + 54000)) { //Se apaga el aire al llevar mucho tiempo en la temperatura deseada //"15 minutos en la temperatura deseada"
    salta_temporizador = 1;
    Realizo_Accion_Aire();
    tiempo_sens_temp_inicio = tiempo_sens_temp_actual;
  }
  if (tiempo_sens_mov_actual >= (tiempo_sens_mov_inicio + 36000)) {   //No se ha detectado movimiento durante 10 min en la estancia
    salta_temporizador = 2;
    Realizo_Accion_Aire();
    tiempo_sens_mov_inicio = tiempo_sens_mov_actual;
  }
  if (tiempo_sens_prox_actual >= (tiempo_sens_prox_inicio + 18000)) { //La ventana lleva abierta 5 min con el aire encendido
    salta_temporizador = 3;
    Realizo_Accion_Aire();
    tiempo_sens_prox_inicio = tiempo_sens_prox_actual;
  }
}

void Envio_Datos_IR_Panasonic(uint16_t code[]) {
  irsend.sendRaw(code, TAM_MES_PAN, FREQ);
}

void Envio_Datos_IR_Mundo_Clima(uint16_t code[]) {
  irsend.sendRaw(code, TAM_MES_MC, FREQ);
}

void Envio_Datos_IR_Fujitsu(uint16_t code[], uint16_t tam) {
  irsend.sendRaw(code, tam, FREQ);
}

void Envio_Datos_Broker() {
  String payload2 = "";
  doc["EstadoVentana"] = message_enc_man.est_ventana;
  doc["EstadoAire"] = message_enc_man.val_ejec;
  serializeJson(doc, payload2);
  client.publish("TFG/ConectarAA", 0, true, (char*)payload2.c_str());
}

void Mostrar_Valores_loop() {
  int board1X = myData_temp.temp;
  int board1Y = myData_temp.hum;
  int board1Z = myData_temp.mov;
  int board2Y = myData_prox.sens_prox; 
  
  //Indicar el estado de los temporizadores o la accion que se realiza en cada momento
}

void Realizar_Copia_Mensaje_Temp() {
  board_temp.id = myData_temp.id;
  board_temp.hum = myData_temp.hum;
  board_temp.temp = myData_temp.temp;
  board_temp.mov = myData_temp.mov;
  board_temp.aviso = myData_temp.aviso;
}

void Realizar_Copia_Mensaje_Prox() {
  board_prox.id = myData_prox.id;
  board_prox.sens_prox = myData_prox.sens_prox;
  board_prox.aviso = myData_prox.aviso;
}

void Realizo_Accion_Aire() {
  int estado = -1; //Si es 0: Enciende, si es 1: Apaga
  int momento_accion = -1; //Si es 0: es manual, si es 1: es programado
  Seleccion_Dispositivo();
  if (nombre_disp == "PANASONIC") {
    Comprueba_Encendido_Apagado_Man();
    temp_ejecucion = message_cam_temp.value_temp;
    Accion_Panasonic(estado_encendido, temp_ejecucion);

  } else if (nombre_disp == "MUNDO CLIMA") {
    Comprueba_Encendido_Apagado_Man();
    temp_ejecucion = message_cam_temp.value_temp;
    Accion_Mundo_Clima(estado_encendido, temp_ejecucion);

  } else if (nombre_disp == "FUJITSU") {
    Comprueba_Encendido_Apagado_Man();
    temp_ejecucion = message_cam_temp.value_temp;
    Accion_Fujitsu(estado_encendido, temp_ejecucion);

  } else {
    res = -1;
  }

  Envio_Datos_Broker();
}

void Comprueba_Encendido_Apagado_Man() {
  if (message_enc_man.val_ejec == 1 && message_apa_man.val_ejec == 0) { //Esta activado el encendido
    estado_encendido = 0;
  } else if (message_apa_man.val_ejec == 1 && message_enc_man.val_ejec == 0) { //Esta encendido el apagado
    estado_encendido = 1;
  }
}

void Comprueba_Sensores() {
  Comprueba_Timer();
  if ((board_temp.temp < temp_ejecucion + 3) && (board_temp.temp > temp_ejecucion - 3) && (board_prox.sens_prox == 0 && salta_temporizador == 0)) { //Todo va bien
    estado_sensores = 0;
    message_enc_man.est_ventana = 0;
    message_enc_man.val_ejec = 0;
    message_apa_man.val_ejec = 1;
  } else if ((board_temp.temp > temp_ejecucion + 3) || (board_temp.temp < temp_ejecucion - 3) && (temp_ejecucion != 0)) { //Temperatura +-3 de la estableciada
    estado_sensores = 1;
    message_enc_man.val_ejec = 1;
    message_apa_man.val_ejec = 0;
  } else if (board_prox.sens_prox == 1) { //Ventana abierta
    estado_sensores = 2;
    message_enc_man.est_ventana = 1;
    message_enc_man.val_ejec = 0;
    message_apa_man.val_ejec = 1;
  } else if (salta_temporizador != 0) { //No se ha detectado movimiento en 5 min
    estado_sensores = 3;
    message_enc_man.val_ejec = 0;
    message_apa_man.val_ejec = 1;
  } else {       //Hay problemas con la lectura de datos de los sensores
    estado_sensores = -1;
  }
}

void Comprueba_Fecha() {
  if (message_enc_prog.val_ejec == 1) {
    if ((year(fecha_actual) == message_enc_prog.val_fecha_ano) &&
        (month(fecha_actual) == message_enc_prog.val_fecha_mes) &&
        (day(fecha_actual) == message_enc_prog.val_fecha_dia) &&
        (hour(fecha_actual) == message_enc_prog.val_reloj_hora) &&
        (minute(fecha_actual) >= message_enc_prog.val_reloj_min)) {

      message_enc_man.val_ejec = 1;
      message_apa_man.val_ejec = 0;
      message_cam_temp.value_temp = message_enc_prog.val_temp;
      Realizo_Accion_Aire();
    }
  }

  if (message_apa_prog.val_ejec == 1) {
    if ((year(fecha_actual) == message_apa_prog.val_fecha_ano) &&
        (month(fecha_actual) == message_apa_prog.val_fecha_mes) &&
        (day(fecha_actual) == message_apa_prog.val_fecha_dia) &&
        (hour(fecha_actual) == message_apa_prog.val_reloj_hora) &&
        (minute(fecha_actual) >= message_apa_prog.val_reloj_min)) {

      message_apa_man.val_ejec = 1;
      message_enc_man.val_ejec = 0;
      message_cam_temp.value_temp = 0;
      Realizo_Accion_Aire();
    }
  }
}

void Lectura_Valores_Broker() {
  deserializeJson(doc, payload);

  marca_aire = doc["Marca_Aire"];
  message_enc_man.val_ejec = doc["Encendido"];
  message_apa_man.val_ejec = doc["Apagado"];

  message_cam_temp.value_temp = doc["Cambio_Temperatura"];
  message_cam_temp.val_ejec = doc["Estado_Temperatura"];

  message_enc_prog.val_fecha_dia = doc["Encendido_Programado_Dia"];
  message_enc_prog.val_fecha_mes = doc["Encendido_Programado_Mes"];
  message_enc_prog.val_fecha_ano = doc["Encendido_Programado_Ano"];
  message_enc_prog.val_reloj_hora = doc["Encendido_Programado_Hora"];
  message_enc_prog.val_reloj_min = doc["Encendido_Programado_Min"];
  message_enc_prog.val_temp = doc["Encendido_Programado_Temp"];
  message_enc_prog.val_ejec = doc["Encendido_Programado_Ejec"];

  message_apa_prog.val_fecha_dia = doc["senApagado_Programado_Dia"];
  message_apa_prog.val_fecha_mes = doc["Apagado_Programado_Mes"];
  message_apa_prog.val_fecha_ano = doc["Apagado_Programado_Ano"];
  message_apa_prog.val_reloj_hora = doc["Apagado_Programado_Hora"];
  message_apa_prog.val_reloj_min = doc["Apagado_Programado_Min"];
  message_apa_prog.val_ejec = doc["Apagado_Programado_Ejec"];

}

void Seleccion_Dispositivo() {
  switch (marca_aire) {
    case 0: //"PANASONIC"
      nombre_disp = "PANASONIC";
      break;

    case 1: //"MUNDO CLIMA"
      nombre_disp = "MUNDO CLIMA";
      break;

    case 2: //"FUJITSU"
      nombre_disp = "FUJITSU";
      break;

    default:
      nombre_disp = "";
      break;
  }
}

void Accion_Panasonic(int accion, int val_temp) {
  int i = 0;
  switch (accion) {
    //Primero 2 casos son los de apagado y encendido
    case 0:
      Temperatura_Panasonic(val_temp);
      Reinicia_Timer();
      Envio_Datos_IR_Panasonic(code_temp_PANASONIC);
      res = 0;
      break;

    case 1:
      Temperatura_Panasonic(val_temp);
      for (i = 0; i < TAM_MES_PAN; i++) {
        code_apaga_PANASONIC[i] = raw_PAN_PARA_APAGADO[i];
      }
      Envio_Datos_IR_Panasonic(code_apaga_PANASONIC);
      res = 0;
      break;

    default:
      res = -1;
      break;
  }
}

void Accion_Mundo_Clima(int accion, int val_temp) {
  int i = 0;
  switch (accion) {
    case 0:
      Temperatura_Mundo_Clima(val_temp);
      Reinicia_Timer();
      Envio_Datos_IR_Mundo_Clima(code_temp_MC);
      res = 0;
      break;

    case 1:
      Temperatura_Mundo_Clima(val_temp);
      for (i = 0; i < TAM_MES_MC; i++) {
        code_apaga_MC[i] = raw_MC_PARA_APAGADO[i];
      }
      Envio_Datos_IR_Mundo_Clima(code_apaga_MC);
      res = 0;
      break;

    default:
      res = -1;
      break;

  }
}

void Accion_Fujitsu(int accion, int val_temp) {
  int i = 0;
  uint16_t tam_enc = TAM_MES_ENC_FUJ;
  uint16_t tam_apa = TAM_MES_APA_FUJ;
  switch (accion) {
    //Primero 2 casos son los de apagado y encendido
    case 0:
      Temperatura_Fujitsu(val_temp);
      Reinicia_Timer();
      Envio_Datos_IR_Fujitsu(code_temp_FUJITSU, tam_enc);
      res = 0;
      break;

    case 1:
      for (i = 0; i < TAM_MES_APA_FUJ; i++) {
        code_apaga_FUJITSU[i] = FUJ_APAGADO[i];
      }
      Envio_Datos_IR_Fujitsu(code_apaga_FUJITSU, tam_apa);
      res = 0;
      break;

    default:
      res = -1;
      break;
  }
}

void Asigna_Valor_Panasonic(uint16_t origen[]) {
  int i = 0;
  for (i = 0; i < TAM_MES_PAN; i++) {
    code_temp_PANASONIC[i] = origen[i];
  }
}

void Temperatura_Panasonic(int val_temp) {
  switch (val_temp) {
    case 18:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_18);
      res = 0;
      break;

    case 19:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_19);
      res = 0;
      break;

    case 20:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_20);
      res = 0;
      break;

    case 21:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_21);
      res = 0;
      break;

    case 22:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_22);
      res = 0;
      break;

    case 23:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_23);
      res = 0;
      break;

    case 24:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_24);
      res = 0;
      break;

    case 25:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_25);
      res = 0;
      break;

    case 26:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_26);
      res = 0;
      break;

    case 27:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_27);
      res = 0;
      break;

    case 28:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_28);
      res = 0;
      break;

    case 29:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_29);
      res = 0;
      break;

    case 30:
      Asigna_Valor_Panasonic(raw_PAN_TEMP_30);
      res = 0;
      break;

    default:
      res = -1;
      break;

  }
}

void Asigna_Valor_Mundo_Clima(uint16_t origen[]) {
  int i = 0;
  for (i = 0; i < TAM_MES_MC; i++) {
    code_temp_MC[i] = origen[i];
  }
}

void Temperatura_Mundo_Clima(int val_temp) {
  switch (val_temp) {
    case 18:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_18);
      res = 0;
      break;

    case 19:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_19);
      res = 0;
      break;

    case 20:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_20);
      res = 0;
      break;

    case 21:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_21);
      res = 0;
      break;

    case 22:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_22);
      res = 0;
      break;

    case 23:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_23);
      res = 0;
      break;

    case 24:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_24);
      res = 0;
      break;

    case 25:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_25);
      res = 0;
      break;

    case 26:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_26);
      res = 0;
      break;

    case 27:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_27);
      res = 0;
      break;

    case 28:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_28);
      res = 0;
      break;

    case 29:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_29);
      res = 0;
      break;

    case 30:
      Asigna_Valor_Mundo_Clima(raw_MC_TEMP_30);
      res = 0;
      break;

    default:
      res = -1;
      break;

  }
}

void Asigna_Valor_Fujitsu(uint16_t origen[]) {
  int i = 0;
  for (i = 0; i < TAM_MES_ENC_FUJ; i++) {
    code_temp_FUJITSU[i] = origen[i];
  }
}

void Temperatura_Fujitsu(int val_temp) {
  switch (val_temp) {
    case 18:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_18);
      res = 0;
      break;

    case 19:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_19);
      res = 0;
      break;

    case 20:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_20);
      res = 0;
      break;

    case 21:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_21);
      res = 0;
      break;

    case 22:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_22);
      res = 0;
      break;

    case 23:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_23);
      res = 0;
      break;

    case 24:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_24);
      res = 0;
      break;

    case 25:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_25);
      res = 0;
      break;

    case 26:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_26);
      res = 0;
      break;

    case 27:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_27);
      res = 0;
      break;

    case 28:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_28);
      res = 0;
      break;

    case 29:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_29);
      res = 0;
      break;

    case 30:
      Asigna_Valor_Fujitsu(raw_FUJ_TEMP_30);
      res = 0;
      break;

    default:
      res = -1;
      break;

  }
}
