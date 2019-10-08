/***********
Sketch for Feather MO
Current Sketch assumes that you have a capacitive moisture sensor on A0, see Define MOI_MAX/MIN below

************/
#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
#include <Adafruit_GPS.h>

#define MASTER 0//Binary, the master unit will be the one with the GSM module in any given set of instalations
                  //the sub modules will send thier data to the master module to then have it all commited.
                  //There should also eventually be a setting for non-Lora modules that exist on thier own
                  //and (ideally) a set for non-gsm modules that operate over LoRaWAN

#define HAS_SENSOR 1//On the offchance for some reason a GSM enabled unit doesn't have a moisture sensor (for example if it's only acting as a relay for another one in an area with no coverage), 
                    //we'll keep the moisture sensor code inside of one of these. This will also make switching out to the I2C style or resistive style sensors later easier,by allowing another value if that becomes neccesary.
#define HAS_GPS 1//This unit has GPS
#define GPSSerial Serial1 //This needs to be a macro and can't be defined in an if statment. There's probably a better way
                          //to do this, that I don't know about.
#define GPSECHO false
#define LOCATION_CODE "LAT:38.950503,LON:-92.395701" //TODO This will need to be defined either as a numerical value that the recieving backend will understand, or as an array of [LAT, LONG]. Either this or the HAS_GPS flag need to be enabled
#define LOC_CODE_LEN 28
#define MOI_MAX = 846//The maximum capacitive value you have observed for the current moisture sensor completly dry. Due to varience in the sensors and wires, this may vary wqith the unit.
#define MOI_MIN = 371//The minimum value observed by submerging the sensor to it's depth line in water

//for feather m0 RFM9x
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
//Must match all RX's freq!
#define RF95_FREQ 915.0
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

int moisture = 0;
uint32_t timer = millis();
Adafruit_GPS GPS(&GPSSerial);
void setup() {
  //START : Setup code for LoRa Radio, taken from older sender sketch
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(115200);
  delay(100);
 
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
 
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
   
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  //END LoRa Setup

  //START: GPS Setup Code
  if(HAS_GPS){
    GPS.begin(9600);
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
    GPS.sendCommand(PGCMD_ANTENNA);
    delay(1000);
    GPSSerial.println(PMTK_Q_RELEASE);
  }
  //END GPS Setup  
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop() {
  if(HAS_GPS){
    char c = GPS.read();
    if(GPSECHO){
      if(c){
      Serial.print(c);
      }
    }
    if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    Serial.println(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
    }
    // if millis() or timer wraps around, we'll just reset it
    if (timer > millis()){
      timer = millis();
    }
    if (millis() - timer > 10000) {
      timer = millis(); // reset the timer
      Serial.print("\nTime: ");
      Serial.print(GPS.hour, DEC); Serial.print(':');
      Serial.print(GPS.minute, DEC); Serial.print(':');
      Serial.print(GPS.seconds, DEC); Serial.print('.');
      Serial.println(GPS.milliseconds);
      Serial.print("Date: ");
      Serial.print(GPS.day, DEC); Serial.print('/');
      Serial.print(GPS.month, DEC); Serial.print("/20");
      Serial.println(GPS.year, DEC);
      Serial.print("Fix: "); Serial.print((int)GPS.fix);
      Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
      if (GPS.fix) {
        Serial.print("Location: ");
        Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
        Serial.print(", ");
        Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
        Serial.print("Speed (knots): "); Serial.println(GPS.speed);
        Serial.print("Angle: "); Serial.println(GPS.angle);
        Serial.print("Altitude: "); Serial.println(GPS.altitude);
        Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
      }
    }
  
  }
  if(!MASTER){//This designates this unit as just a moisture sensor that will transmit to the master with the GSM module
    Serial.println("Sending to rf95_server");
    // Send a message to rf95_server
    moisture = analogRead(0);//Anolog reading of capaicitive moisture sensor
    Serial.print("\nLocal Moisture: ");
    Serial.print(moisture);

    char radiopacket[80];
    if(HAS_GPS){
      char temp[5];
      temp[0]='M';
      temp[1]=':';
      itoa(moisture,temp+2,10);//this shoudl be 3 characters
      String rp(temp);
      String timecode();
      rp.concat("t:");
      rp.concat(GPS.hour);
      rp.concat(":");
      rp.concat(GPS.minute);
      rp.concat(":");
      rp.concat(GPS.seconds);
      rp.concat(".");
      rp.concat(GPS.milliseconds);
      rp.concat("d:");
      rp.concat(GPS.day);
      rp.concat(GPS.month);
      rp.concat(GPS.year);
      rp.concat("lat:");
      rp.concat(GPS.lat);
      rp.concat("lon:");
      rp.concat(GPS.lon);
      Serial.println(rp);
      Serial.println(radiopacket);
      for(int i=0;i<80;i++){
        radiopacket[i]=rp[i];      
      }
    }      
    else{
      //radiopacket = (char*)malloc(sizeof(char));
      radiopacket[0]='M';
      radiopacket[1]=':';
      itoa(moisture,radiopacket+2,10);//Should be 3 characters, so radiopacket[2-4]
      for(int i=0;i<LOC_CODE_LEN;i++){
        radiopacket[5+i]=LOCATION_CODE[i];
      }
    }
    
    
    Serial.print("Sending :"); Serial.println(radiopacket);

    Serial.println("Sending..."); delay(10);
    rf95.send((uint8_t *)radiopacket, sizeof(radiopacket));
   
    Serial.println("Waiting for packet to complete..."); delay(10);
    rf95.waitPacketSent();
    //Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
   
    Serial.println("Waiting for reply..."); delay(10);
    if (rf95.waitAvailableTimeout(5000))
    { 
      // Should be a reply message for us now   
      if (rf95.recv(buf, &len))
     {
//        digitalWrite(LED, HIGH);
        Serial.print("Got reply: ");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);    
        delay(250);
//        digitalWrite(LED, LOW);
      }
      else
      {
        Serial.println("Receive failed");
      }
    }
    delay(30*1000);
  }
  else if(MASTER){//Designates unit as having a GSM module.  
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      //digitalWrite(LED, HIGH);
      //RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Remote Moisture: ");
      Serial.println((char*)buf);
      //Serial.print("RSSI: ");
      //Serial.println(rf95.lastRssi(), DEC);
      if(HAS_SENSOR){
      moisture = analogRead(0);//Anolog reading of capaicitive moisture sensor
      Serial.print("Local Moisture: ");
      Serial.print(moisture);
      }
      
      delay(10);
      // Send a reply
      delay(200); // may or may not be needed
      uint8_t data[] = "recieved";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("\nSent a reply\n");
    }
    else
    {
      delay(5000);
    }
  }
}
  
