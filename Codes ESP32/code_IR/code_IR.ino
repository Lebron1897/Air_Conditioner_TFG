#include "Metodos.h"

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.begin(9600);
  setup_cliente();
  
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  Reinicia_Timer();
}

void loop() {
  // put your main code here, to run repeatedly:
  //Mostrar_valores_loop();
  Actualiza_Fecha_Hora();
  Actualiza_Timer();
  Comprueba_Sensores();
  Comprueba_Fecha();
    
}





//Recibir los valores de los sensores de las otras placas
//Realizar una maquina de estados que ira realizando una función distinta dependiendo de los valores que lleguen
//Realizar otra maquina de estados para la posible configuración que se le de desde el broker
//Realizar las lecturas de valores del broker
//Realizar el envio de datos al broker
//Configurar el IR de envio de datos y las conversiones de codigo hexadecimal a codigo RAW que lea el aire acondicionado
