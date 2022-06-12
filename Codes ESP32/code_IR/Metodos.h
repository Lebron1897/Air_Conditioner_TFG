#include <esp_now.h>
#include <IRremoteESP8266.h>
#include <ArduinoJson.h>
#include <IRsend.h>
#include <string.h>
#include "messages.h"
#include "EspMQTTClient.h"
#include "RTClib.h"

#define TAM_MES_PAN 439
#define TAM_MES_MC 67
#define TAM_MES_ENC_FUJ 259
#define TAM_MES_APA_FUJ 115

//Declaracion de las estructuras de datos
typedef struct message_IR{
  int id;
}message_IR;

typedef struct message_temp {
  int id;
  int hum;
  int temp;
  int mov;
  int aviso;
}message_temp;

typedef struct message_prox {
  int id;
  int sens_prox;
  int aviso;
}message_prox;

//Comienza la delclaracion de funciones
void setup_cliente();
void setup_wifi() ;
void reconnect();
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
void callback_json(char* topic, byte* payload, unsigned int length);
void Actualiza_Fecha_Hora();
void Actualiza_Timer();
void Reinicia_Timer();
void Comprueba_Timer();
void Envio_Datos_IR_Panasonic(uint16_t code[]);    //FALTA POR HACERLA
void Envio_Datos_IR_Mundo_Clima(uint16_t code[]);    //FALTA POR HACERLA
void Envio_Datos_IR_Fujitsu(uint16_t code[], uint16_t tam);    //FALTA POR HACERLA
void Envio_Datos_Broker();        //FALTA POR HACERLA
void Mostrar_Valores_loop();
void Realizo_Accion_Aire();
void Realizar_Copia_Mensaje_Temp();
void Realizar_Copia_Mensaje_Prox();
void Comprueba_Encendido_Apagado_Man();
void Comprueba_Sensores();
void Comprueba_Fecha();
void Lectura_Valores_Broker();
void Seleccion_Dispositivo();
void Asigna_Valor_Panasonic(uint16_t origen[]);
void Asigna_Valor_Mundo_Clima(uint16_t origen[]);
void Asigna_Valor_Fujitsu(uint16_t origen[]);
void Temperatura_Panasonic(int val_temp);
void Temperatura_Mundo_Clima(int val_temp);
void Temperatura_Fujitsu(int val_temp);
void Accion_Panasonic(int accion, int val_temp);
void Accion_Mundo_Clima(int accion, int val_temp);
void Accion_Fujitsu(int accion, int val_temp);
