#define DOOR_SENSOR_PIN  23  // ESP32 pin GIOP23 connected to the OUTPUT pin of door sensor
#define LED_PIN          17  // ESP32 pin GIOP17 connected to LED's pin

#define TIME_MAX_OPEN 120000  //2 min time max open before send a message or turn off the air conditioner

int doorState;
int inputPin = 26; // for ESP32 microcontroller

void setup() {
  
  pinMode(inputPin, INPUT);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
  pinMode(LED_PIN, OUTPUT);               // set ESP32 pin to output mode
  Serial.begin(115200);   //Se inicia la comunicación serial 
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
  
  delay(5000); //PAra realizar lecturas cada medio segundo
}

//Faltaria por generar un timer, de manera que cuando el led este encendido el timer este contand
//Si pasa de un determinado tiempo de encendido, entonces se enviara un señal con un aviso de que se debe de apagar el aire.
//En caso contrario, el temporizador estará apagado o realizando envios cada x segundos
