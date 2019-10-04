#include <SPI.h>
#include <Wire.h>

int moisture = 0;
void setup() {
  // put your setup code here, to run once:
  
}

void loop() {
  moisture = analogRead(0);//Anolog reading of capaicitive moisture sensor
  Serial.print("Moisture: ");
  Serial.print(moisture);

}
