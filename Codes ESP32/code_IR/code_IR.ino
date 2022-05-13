#include <esp_now.h>
#include <WiFi.h>

typedef struct message_temp {
  int id;
  int hum;
  int temp;
  int mov;
}message_temp;

typedef struct message_prox {
  int id;
  int sens_prox;
}message_prox;

// Create a struct_message called myData
message_temp myData_temp;
message_prox myData_prox;

// Create a structure to hold the readings from each board
message_temp board_temp;
message_prox board_prox;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData_temp, incomingData, sizeof(myData_temp));
  memcpy(&myData_prox, incomingData, sizeof(myData_prox));
  if(myData_temp.id == 1){
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
    Serial.printf("Humidity: %d \n", myData_temp.hum);
    Serial.println();
  }else if(myData_prox.id == 2){
    Serial.printf("Board ID %u: %u bytes\n", myData_prox.id, len);
    if (myData_prox.sens_prox == HIGH) {
      Serial.println("Ventana abierta");
      //digitalWrite(LED_PIN, HIGH); // turn on LED
    } else {
      Serial.println("Ventana cerrada");
      //digitalWrite(LED_PIN, LOW);  // turn off LED
    }
      Serial.println();
  }
}

void setup() {
  // put your setup code here, to run once:
  //Initialize Serial Monitor
  Serial.begin(115200);
  
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // put your main code here, to run repeatedly:
  int board1X = myData_temp.temp;
  int board1Y = myData_temp.hum;
  int board1Z = myData_temp.mov;
  int board2Y = myData_prox.sens_prox;

  delay(10000);  
}

//Recibir los valores de los sensores de las otras placas
//Realizar una maquina de estados que ira realizando una función distinta dependiendo de los valores que lleguen
//Realizar otra maquina de estados para la posible configuración que se le de desde el broker
//Realizar las lecturas de valores del broker
//Realizar el envio de datos al broker
//Configurar el IR de envio de datos y las conversiones de codigo hexadecimal a codigo RAW que lea el aire acondicionado
