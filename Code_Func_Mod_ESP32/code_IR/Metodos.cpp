#include <Time.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Code_Remotes.h"
#include "Metodos.h"

#define ENCENDIDO 0
#define APAGADO 1

//Declaracion de variables para MQTT
const char* mqtt_server = "broker.hivemq.com"; //TFG/ConectarAA
//const char* mqtt_server = "192.168.18.231"; //TFG/ConectarAA
const int mqttPort = 1883;
const char* ssid = "Network id";         
const char* password = "NEtwork Password";   
const char* mqttUser = "";
const char* mqttPassword = "";

WiFiClient espClient;
PubSubClient client(espClient);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

const uint16_t PinIR = 18;
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
uint8_t last_temp = 0;
int salta_temporizador = 0;
int marca_aire = -1;
int code_temp = -1;
int code_apagado = -1;
int estado_encendido = -1;
int estado_sensores = -1;
int temp_ejecucion = 0;
int res = -1;
byte *payload;
StaticJsonDocument <1024> doc_rec;
StaticJsonDocument <1024> doc_envio;
IRsend irsend(PinIR);

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
  client.setBufferSize(1024);
  if (!client.connected()) {
    reconnect();
  }

  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  delay(2000);
}

void setup_wifi() {
  WiFi.mode(WIFI_AP_STA);
  
  delay(10);
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
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32 Client", mqttUser, mqttPassword)) {
      Serial.println("connected");

      client.subscribe("/TFG/ConectarAA");
      Serial.println("Se ha realizado la conexion al topic"); 

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup_esp_now(){
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);

  memcpy(&myData_temp, incomingData, sizeof(myData_temp));
  memcpy(&myData_prox, incomingData, sizeof(myData_prox));
  if(myData_temp.aviso == 1){
    Realizar_Copia_Mensaje_Temp();
    myData_temp.aviso = 0;
  }else if(myData_prox.aviso == 1){
    Realizar_Copia_Mensaje_Prox();
    myData_prox.aviso = 0;
  }

  if (myData_temp.id == 1) {
    Serial.printf("Board ID %u: %u bytes\n", myData_temp.id, len);
    // Update the structures with the new incoming data
    Serial.printf("Temperatura: %dºC \n", myData_temp.temp);
    Serial.printf("Humidadad: %d \n", myData_temp.hum);
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

  deserializeJson(doc_rec, payload);

  marca_aire = doc_rec["Marca_Aire"];
  
  message_enc_man.val_ejec = doc_rec["Encendido"];
  message_apa_man.val_ejec = doc_rec["Apagado"];
  
  message_cam_temp.value_temp = doc_rec["Cambio_Temperatura"];
  message_cam_temp.val_ejec = doc_rec["Estado_Temperatura"];

  message_enc_prog.val_fecha_dia = doc_rec["Encendido_Programado_Dia"];
  message_enc_prog.val_fecha_mes = doc_rec["Encendido_Programado_Mes"];
  message_enc_prog.val_fecha_ano = doc_rec["Encendido_Programado_Ano"];
  message_enc_prog.val_reloj_hora = doc_rec["Encendido_Programado_Hora"];
  message_enc_prog.val_reloj_min = doc_rec["Encendido_Programado_Min"];
  message_enc_prog.val_temp = doc_rec["Encendido_Programado_Temp"];
  message_enc_prog.val_ejec = doc_rec["Encendido_Programado_Ejec"];

  message_apa_prog.val_fecha_dia = doc_rec["Apagado_Programado_Dia"];
  message_apa_prog.val_fecha_mes = doc_rec["Apagado_Programado_Mes"];
  message_apa_prog.val_fecha_ano = doc_rec["Apagado_Programado_Ano"];
  message_apa_prog.val_reloj_hora = doc_rec["Apagado_Programado_Hora"];
  message_apa_prog.val_reloj_min = doc_rec["Apagado_Programado_Min"];
  message_apa_prog.val_ejec = doc_rec["Apagado_Programado_Ejec"];

  temp_ejecucion = message_cam_temp.value_temp;
  if(temp_ejecucion != 0){
    last_temp = temp_ejecucion;
  }
  
  if(message_enc_man.val_ejec != 0 || message_apa_man.val_ejec != 0){
    do{
      Realizo_Accion_Aire();
    }
    while (res == -1);
  }else{
    Mostrar_Valores_loop();
  }
}

void Actualiza_Cliente_loop(){
  client.loop();
}

void Actualiza_Fecha_Hora() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  SetLocalTime();
  fecha_actual = now();
  Serial.println("Genero la fecha inicial");
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
  Actualiza_Timer();
    if(message_enc_man.val_ejec == 1){
    //Sensor de movimiento
    if(board_temp.mov == 1){
      tiempo_sens_mov_inicio = tiempo_sens_mov_actual;
    }else{
      if(tiempo_sens_mov_actual >= (tiempo_sens_mov_inicio + 900000)){
        message_enc_man.val_ejec = 0;
        message_apa_man.val_ejec = 1;
        Realizo_Accion_Aire();
      }else{
        Actualiza_Timer();
      }
    }

    //Sensor de proximidad
    if(board_prox.sens_prox == 0){
      tiempo_sens_prox_inicio = tiempo_sens_prox_actual;
    }else{
      if(tiempo_sens_prox_actual >= (tiempo_sens_prox_inicio + 300000)){
        Envio_Datos_Broker();
      }else{
        Actualiza_Timer();
      }
    }

    //Sensor de temperatura
    if(board_temp.temp != message_cam_temp.value_temp){
      tiempo_sens_temp_inicio = tiempo_sens_temp_actual;
    }else{
      if(tiempo_sens_temp_actual >= (tiempo_sens_temp_inicio + 900000)){
        message_enc_man.val_ejec = 0;
        message_apa_man.val_ejec = 1;
        Realizo_Accion_Aire();
      }else{
        Actualiza_Timer();
      }
    }
  }else if(message_apa_man.val_ejec == 1){
    Reinicia_Timer();
  }
  delay(3000);
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
  doc_envio["EstadoVentana"] = board_prox.sens_prox;
  doc_envio["EstadoAire"] = message_enc_man.val_ejec;
  serializeJson(doc_envio, payload2);
  client.publish("/TFG/Estados",(char*)payload2.c_str());
}

void Mostrar_Valores_loop() {
  Serial.print("Marca Aire: ");
  Serial.println(marca_aire);
  Serial.println();
  
  Serial.print("Valor Encendido Manual: ");
  Serial.println(message_enc_man.val_ejec);
  Serial.println();
  
  Serial.print("Valor Apagado Manual: ");
  Serial.println(message_apa_man.val_ejec);
  Serial.println();
  
  Serial.print("Valor Temperatura Manual: ");
  Serial.println(message_cam_temp.value_temp);
  Serial.println(message_cam_temp.val_ejec);
  Serial.println();
  
  Serial.println("Valores Encendido Programado");
  Serial.println("Fecha: ");
  Serial.print(message_enc_prog.val_fecha_dia);
  Serial.print(+ "/") ;
  Serial.print(message_enc_prog.val_fecha_mes);
  Serial.print(+ "/") ;
  Serial.println(message_enc_prog.val_fecha_ano); 

  Serial.println("Tiempo: ");
  Serial.print(message_enc_prog.val_reloj_hora);  
  Serial.print(+ ":") ;
  Serial.println(message_enc_prog.val_reloj_min);
  Serial.print("Valor de Temperatura: ");
  Serial.println(message_enc_prog.val_temp);
  Serial.print("Valor de Ejecucion: ");
  Serial.println(message_enc_prog.val_ejec);
  Serial.println();
  
  Serial.println("Valores Apagado Programado");
  Serial.println("Fecha: ");
  Serial.print(message_apa_prog.val_fecha_dia);
  Serial.print(+ "/") ;
  Serial.print(message_apa_prog.val_fecha_mes);
  Serial.print(+ "/") ;
  Serial.println(message_apa_prog.val_fecha_ano); 

  Serial.println("Tiempo: ");
  Serial.print(message_apa_prog.val_reloj_hora);  
  Serial.print(+ ":") ;
  Serial.println(message_apa_prog.val_reloj_min);
  Serial.print("Valor de Ejecucion: ");
  Serial.println(message_apa_prog.val_ejec);
  
  Serial.println("Fin Recogido Callback");
  Serial.println();
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
  Seleccion_Dispositivo();
  if (nombre_disp == "PANASONIC") {
    Comprueba_Encendido_Apagado_Man();
    if(message_cam_temp.value_temp == 0){
      temp_ejecucion = last_temp;
    }else if(message_cam_temp.value_temp != 0){
      temp_ejecucion = message_cam_temp.value_temp;
    }
    Accion_Panasonic(estado_encendido, temp_ejecucion);

  } else if (nombre_disp == "MUNDO CLIMA") {
    Comprueba_Encendido_Apagado_Man();
    if(message_cam_temp.value_temp == 0){
      temp_ejecucion = last_temp;
    }else if(message_cam_temp.value_temp != 0){
      temp_ejecucion = message_cam_temp.value_temp;
    }
    Accion_Mundo_Clima(estado_encendido, temp_ejecucion);

  } else if (nombre_disp == "FUJITSU") {
    Comprueba_Encendido_Apagado_Man();
    if(message_cam_temp.value_temp == 0){
      temp_ejecucion = last_temp;
    }else if(message_cam_temp.value_temp != 0){
      temp_ejecucion = message_cam_temp.value_temp;
    }
    Accion_Fujitsu(estado_encendido, temp_ejecucion);

  } else {
    res = -1;
  }
  Mostrar_Valores_loop();
  Envio_Datos_Broker();
}

void Comprueba_Encendido_Apagado_Man() {
  if (message_enc_man.val_ejec == 1 && message_apa_man.val_ejec == 0) { //Esta activado el encendido
    estado_encendido = 0;
  } else if (message_apa_man.val_ejec == 1 && message_enc_man.val_ejec == 0) { //Esta encendido el apagado
    estado_encendido = 1;
  }
}

void Comprueba_Fecha() {
  fecha_actual = now();
  if (message_enc_prog.val_ejec == 1) {
    if ((year(fecha_actual) == message_enc_prog.val_fecha_ano) &&
        (month(fecha_actual) == message_enc_prog.val_fecha_mes) &&
        (day(fecha_actual) == message_enc_prog.val_fecha_dia) &&
        (hour(fecha_actual) == message_enc_prog.val_reloj_hora) &&
        (minute(fecha_actual) == message_enc_prog.val_reloj_min) &&
        (message_enc_man.val_ejec != 1)) {

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
        (minute(fecha_actual) == message_apa_prog.val_reloj_min) &&
        (message_apa_man.val_ejec != 1)) {

      message_apa_man.val_ejec = 1;
      message_enc_man.val_ejec = 0;
      message_cam_temp.value_temp = 0;
      Realizo_Accion_Aire();
    }
  }
}

void SetLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  setTime(timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900);
}

void Lectura_Valores_Broker() {
  deserializeJson(doc_rec, payload);

  marca_aire = doc_rec["Marca_Aire"];
  message_enc_man.val_ejec = doc_rec["Encendido"];
  message_apa_man.val_ejec = doc_rec["Apagado"];

  message_cam_temp.value_temp = doc_rec["Cambio_Temperatura"];
  message_cam_temp.val_ejec = doc_rec["Estado_Temperatura"];

  message_enc_prog.val_fecha_dia = doc_rec["Encendido_Programado_Dia"];
  message_enc_prog.val_fecha_mes = doc_rec["Encendido_Programado_Mes"];
  message_enc_prog.val_fecha_ano = doc_rec["Encendido_Programado_Ano"];
  message_enc_prog.val_reloj_hora = doc_rec["Encendido_Programado_Hora"];
  message_enc_prog.val_reloj_min = doc_rec["Encendido_Programado_Min"];
  message_enc_prog.val_temp = doc_rec["Encendido_Programado_Temp"];
  message_enc_prog.val_ejec = doc_rec["Encendido_Programado_Ejec"];

  message_apa_prog.val_fecha_dia = doc_rec["senApagado_Programado_Dia"];
  message_apa_prog.val_fecha_mes = doc_rec["Apagado_Programado_Mes"];
  message_apa_prog.val_fecha_ano = doc_rec["Apagado_Programado_Ano"];
  message_apa_prog.val_reloj_hora = doc_rec["Apagado_Programado_Hora"];
  message_apa_prog.val_reloj_min = doc_rec["Apagado_Programado_Min"];
  message_apa_prog.val_ejec = doc_rec["Apagado_Programado_Ejec"];
}

void Seleccion_Dispositivo() {
  switch (marca_aire) {  
    case 0: //"MUNDO CLIMA"
      nombre_disp = "MUNDO CLIMA";
      break;
      
    case 1: //"PANASONIC"
      nombre_disp = "PANASONIC";
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
        code_apaga_FUJITSU[i] = raw_FUJ_APAGADO[i];
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
