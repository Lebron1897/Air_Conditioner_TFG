#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#define DOOR_SENSOR_PIN  2  // ESP32 pin GIOP2 connected to the OUTPUT pin of door sensor
#define LED_PIN          17  // ESP32 pin GIOP17 connected to LED's pin

#define TIME_MAX_OPEN 120000  //2 min time max open before send a message or turn off the air conditioner

uint8_t broadcastAddress[] = {0xC8, 0xC9, 0xA3, 0xCA, 0xEB, 0x34};

constexpr char WIFI_SSID[] = "OliveNet-1897"; 

int doorState;
int inputPin = 26; // for ESP32 microcontroller

typedef struct message_prox {
    int id;
    int sens_prox;
    int aviso;
} message_prox;

// Create a struct_message called myData
message_prox myData_prox;

// Create peer interface
esp_now_peer_info_t peerInfo;

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
  
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
  pinMode(LED_PIN, OUTPUT);               // set ESP32 pin to output mode
  Serial.begin(115200);   //Se inicia la comunicación serial 

  //Para la comunicacion entre ESP32
  //Esta actua como master para enviar la informacion

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
  
  doorState = digitalRead(DOOR_SENSOR_PIN); // read state
  
  if (doorState == HIGH) {
    Serial.println("Ventana abierta");
    digitalWrite(LED_PIN, HIGH); // turn on LED
  } else {
    Serial.println("Ventana cerrada");
    digitalWrite(LED_PIN, LOW);  // turn off LED
  }

    // Set values to send
  myData_prox.id = 2;
  myData_prox.sens_prox = doorState;
  myData_prox.aviso = 1 ;
  

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData_prox, sizeof(myData_prox));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(10000);
  
}

//Faltaria por generar un timer, de manera que cuando el led este encendido el timer este contand
//Si pasa de un determinado tiempo de encendido, entonces se enviara un señal con un aviso de que se debe de apagar el aire.
//En caso contrario, el temporizador estará apagado o realizando envios cada x segundos
