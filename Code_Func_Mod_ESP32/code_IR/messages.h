#include <string.h>

typedef enum {
    MESSAGE_REJECTED,
    MESSAGE_ENCENDIDO_MANUAL,
    MESSAGE_APAGADO_MANUAL,
    MESSAGE_CAMBIO_TEMPERATURA,
    MESSAGE_ENCENDIDO_PROGRAMADO,
    MESSAGE_APAGADO_PROGRAMADO,
    MESSAGE_CANCELA_PROGRAMADO,
} messageTypes;

#pragma pack(1) //Cambia el alineamiento de datos en memoria a 1 byte.

typedef struct {
    uint8_t command;
} MESSAGE_REJECTED_PARAMETROS;

typedef struct{
    uint8_t val_ejec;
    uint8_t est_ventana;
}MESSAGE_ENCENDIDO_MANUAL_PARAMETROS;

typedef struct{
    uint8_t val_ejec;
}MESSAGE_APAGADO_MANUAL_PARAMETROS;

typedef struct{
    uint16_t value_temp;
    uint8_t val_ejec;
}MESSAGE_CAMBIO_TEMPERATURA_PARAMETROS;

typedef struct{
    uint8_t val_fecha_dia;
    uint8_t val_fecha_mes;
    uint16_t val_fecha_ano;
    uint8_t val_reloj_hora;
    uint8_t val_reloj_min;
    uint8_t val_temp;
    uint8_t val_ejec;
}MESSAGE_ENCENDIDO_PROGRAMADO_PARAMETROS;

typedef struct{
    uint8_t val_fecha_dia;
    uint8_t val_fecha_mes;
    uint16_t val_fecha_ano;
    uint8_t val_reloj_hora;
    uint8_t val_reloj_min;
    uint8_t val_ejec;
}MESSAGE_APAGADO_PROGRAMADO_PARAMETROS;


#pragma pack()  //...Pero solo para los comandos que voy a intercambiar, no para el resto.
