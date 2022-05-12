#include <DHT.h>            //Cargamos la librería DHT

#define DHTTYPE  DHT22      //Definimos el modelo del sensor DHT22
#define DHTPIN    4         //Se define el pin D5 del ESP32 para conectar el sensor DHT22
#define MOVEMENT_PIN 26     //Se define el pin D26 del ESP32 para conectar la PIR de movimiento

int val = LOW;
DHT dht(DHTPIN, DHTTYPE, 22); 

void setup() {
  pinMode(MOVEMENT_PIN, INPUT);
  Serial.begin(115200);   //Se inicia la comunicación serial 
  dht.begin();

}

void loop() {
  //Lecturas del sensor de temperatura y humedad
  float h = dht.readHumidity();     //Se lee la humedad y se asigna el valor a "h"
  float t = dht.readTemperature();  //Se lee la temperatura y se asigna el valor a "t"

  if (isnan(h) || isnan(t)) {
      Serial.println("Fallo en la lectura");
      return;
   }
 
  //Se imprimen las variables del sensor de temperatura y humedad
  Serial.println("Humedad: "); 
  Serial.println(h);
  Serial.println("Temperatura: ");
  Serial.println(t);

  //Lecturas del sensor de movimento e impresion de su valor
  val = digitalRead(MOVEMENT_PIN);
  if (val == HIGH) {
    Serial.println("Movimiento detectado!");
  }
  else {
    Serial.println("No se ha detectado movimiento!");
  }
  delay(5000); //Se puede cambiar por un light-sleep, que deja la placa dormida, pero conectada al wifi
}

//Realizar el envio de los datos cada cierto tiempo a la otra placa
