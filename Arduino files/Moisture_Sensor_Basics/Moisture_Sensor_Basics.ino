/***********
Sketch for Feather MO
Current Sketch assumes that you have a capacitive moisture sensor with a range from  and 

************/
#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>

#define MASTER 0//Binary, the master unit will be the one with the GSM module in any given set of instalations
                  //the sub modules will send thier data to the master module to then have it all commited.
                  //There should also eventually be a setting for non-Lora modules that exist on thier own
                  //and (ideally) a set for non-gsm modules that operate over LoRaWAN

//for feather m0 RFM9x
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
//Must match all RX's freq!
#define RF95_FREQ 915.0
// Blinky on receipt
#define LED 13
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

#define MOI_MAX = 846//The maximum capacitive value you have observed for the current moisture sensor completly dry
#define MOI_MIN = 371//The minimum value observed by submerging the sensor to it's depth line in water

int moisture = 0;
void setup() {
  // put your setup code here, to run once:
  //START : Setup code for LoRa Radio, taken from older sender sketch
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  while (!Serial);
  Serial.begin(9600);
  delay(100);

  Serial.println("Feather LoRa TX Test!");
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
 
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  //END LoRa Setup

  

}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop() {
  moisture = analogRead(0);//Anolog reading of capaicitive moisture sensor
  Serial.print("\nMoisture: ");
  Serial.print(moisture);
  delay(500);


  if(!MASTER){
    Serial.println("Sending to rf95_server");
    // Send a message to rf95_server
    
    int radiopacket = moisture;
    Serial.print("Sending "); Serial.println(radiopacket);

    Serial.println("Sending..."); delay(10);
    rf95.send((uint8_t *) radiopacket, 1);
   
    Serial.println("Waiting for packet to complete..."); delay(10);
    rf95.waitPacketSent();
    // Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
   
    Serial.println("Waiting for reply..."); delay(10);
    if (rf95.waitAvailableTimeout(1000))
    { 
      // Should be a reply message for us now   
      if (rf95.recv(buf, &len))
     {
        digitalWrite(LED, HIGH);
        Serial.print("Got reply: ");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);    
        delay(250);
        digitalWrite(LED, LOW);
      }
      else
      {
        Serial.println("Receive failed");
      }
    }
    else
    {
      Serial.println("No reply, is there a master around?");
    }
    delay(1000);
  }
}
