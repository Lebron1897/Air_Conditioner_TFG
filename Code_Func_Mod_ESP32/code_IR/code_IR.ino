#include "Metodos.h"

void setup() {
  Serial.begin(115200);
  
  setup_wifi();
  setup_cliente();
  setup_esp_now();
  Actualiza_Fecha_Hora();
}

void loop() {
  Comprueba_Timer();
  Comprueba_Fecha();
  Actualiza_Cliente_loop();
}
