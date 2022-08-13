
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <DHT.h>            //Cargamos la librería DHT

#define DHTTYPE  DHT22      //Definimos el modelo del sensor DHT22
#define DHTPIN    14         //Se define el pin D14 del ESP32 para conectar el sensor DHT22
#define MOVEMENT_PIN 21     //Se define el pin D26 del ESP32 para conectar la PIR de movimiento

uint8_t broadcastAddress[] ={0xC8, 0xC9, 0xA3, 0xCA, 0xEB, 0x34};
constexpr char WIFI_SSID[] = "OliveNet-1897"; 

// Structure example to send data
// Must match the receiver structure
typedef struct message_temp {
    int id; // must be unique for each sender board
    int temp;
    int hum;
    int mov;
    int aviso;
} message_temp;

// Create a struct_message called myData
message_temp myData_temp;

// Create peer interface
esp_now_peer_info_t peerInfo;

int val_mov = LOW;
DHT dht(DHTPIN, DHTTYPE, 22); 

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

void setup() {
  pinMode(MOVEMENT_PIN, INPUT);
  Serial.begin(115200);   //Se inicia la comunicación serial 
  dht.begin();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after


  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

}

void loop() {
  //Lecturas del sensor de temperatura y humedad
  float val_hum = dht.readHumidity();     //Se lee la humedad y se asigna el valor a "h"
  float val_tem = dht.readTemperature();  //Se lee la temperatura y se asigna el valor a "t"

  if (isnan(val_hum) || isnan(val_tem)) {
      Serial.println("Fallo en la lectura");
      //return;
   }
 
  //Se imprimen las variables del sensor de temperatura y humedad
  Serial.println("Humedad: "); 
  Serial.println(val_hum);
  Serial.println("Temperatura: ");
  Serial.println(val_tem);

  //Lecturas del sensor de movimento e impresion de su valor
  val_mov = digitalRead(MOVEMENT_PIN);
  if (val_mov == HIGH) {
    Serial.println("Movimiento detectado!");
  }
  else {
    Serial.println("No se ha detectado movimiento!");
  }

  // Set values to send
  myData_temp.id = 1;
  myData_temp.temp = val_tem;
  myData_temp.hum = val_hum;
  myData_temp.mov = val_mov;
  myData_temp.aviso = 1;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData_temp, sizeof(myData_temp));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(10000);
 
  //delay(5000); //Se puede cambiar por un light-sleep, que deja la placa dormida, pero conectada al wifi
}

//Realizar el envio de los datos cada cierto tiempo a la otra placa
