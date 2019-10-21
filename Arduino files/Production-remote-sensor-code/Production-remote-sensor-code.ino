#define POST_URL //These two need to be filled in. Removed from public Git.
#define PHONE_NUM 
#include <Adafruit_FONA.h>
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
#define FONA_KEY 9 //Cycle down for 2 seconds and then back up to power GSM/GPS/SMS module on or off
#define FONA_PS 10 //High if up, low if down.
#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
#include <ArduinoJson.h>


#define HAS_GPS 1//This unit has GPS

#define GPSECHO false
#define MOI_MAX 846//The maximum capacitive value you have observed for the current moisture sensor completly dry. Due to varience in the sensors and wires, this may vary wqith the unit.
#define MOI_MIN 371//The minimum value observed by submerging the sensor to it's depth line in water
// this is a large buffer for replies
char replybuffer[255];

HardwareSerial *fonaSerial = &Serial1;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

uint8_t type;

int moisture = 0;
void setup(){
  while(!Serial)//Testing step. The program starts before the serial connection initializes otherwise.
  
  Serial.println("Debug Step 1");
  Serial.println(digitalRead(FONA_PS));
  if(digitalRead(FONA_PS)==LOW){//Powers on FONA if powered off
    digitalWrite(5,LOW);
    digitalWrite(6,LOW);
    digitalWrite(9,LOW);
    delay(3000);
    digitalWrite(5,HIGH);
    digitalWrite(6,HIGH);
    digitalWrite(9,HIGH);
    delay(3000);
    digitalWrite(5,LOW);
    digitalWrite(6,LOW);
    digitalWrite(9,LOW);
    delay(3000);
    
  }
  Serial.println("Debug Step 2");
  Serial.begin(115200);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));
  Serial.println("Debug Step 3");

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  Serial.println("Debug Step 4");

  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  fona.enableGPS(true);
  fona.sendSMS(PHONE_NUM, "Fona Online!");
  Serial.println("Debug Step 5");
}
void loop(){
  int moisture = analogRead(0);
  Serial.println("Debug Step 6");
  /*Expected POST data
  {"gps_data":"1,1,20191017012737.000,38.950548,-92.395767,185.700,0.15,96.2,1,,1.3,2.2,1.7,,9,8,,,40,,",
   "current_moisture":341,
   "min_moisture":124,
   "max_moisture":741,
   "location":"Rhett's Run",
   "CCID":12345678
  }
  */
  //uint16_t statuscode;
  //int16_t length;
  //char url[80];
  //char data[80];
  //example POST fona.HTTP_POST_start(url, F("application/json"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length))
  char CCID[20];
  fona.getSIMCCID(CCID);
  char location[20] = "Rhett's Run";
  fona.enableGPS(true);//I'm repeating this because I've sometimes had the modules disable it on power off.
  for(int i=0;i<20;i++){
    int GPS_Status = fona.GPSstatus();
    if(GPS_Status==0){
      fona.enableGPS(true);
    }
    if(GPS_Status==1){
      delay(16000);
    }
  }
  Serial.println("Debug Step 7");
  char gpsdata[120];
  fona.getGPS(0, gpsdata, 120);
  StaticJsonDocument<400> datapoint;
  //JsonObject& datapoint = jsonBuffer.createObject();
  datapoint["gps"]=gpsdata;
  datapoint["current_moisture"]=moisture;
  datapoint["min_moisture"]=MOI_MIN;
  datapoint["max_moisture"]=MOI_MAX;
  datapoint["location"]=location;
  datapoint["CCID"]=CCID;
  char clock_buffer[23];
  fona.getTime(clock_buffer, 23);
  String time_str = String(clock_buffer);
  datapoint["_id"]="Test_Insert_"+ time_str;
  //Serial.println(datapoint);
  String datapoint_serial;
  serializeJson(datapoint, Serial);
  serializeJson(datapoint, datapoint_serial);
  const char *datapoint_c_str = datapoint_serial.c_str();
  //Post data to website
  //String datapoint = "{\"gps\":"+gps_data+",\"current_moisture\":"+moisture+",\"min_moisture\":"+
  uint16_t statuscode;
  int16_t length;
  Serial.println("Debug Step 8");

  Serial.flush();
  Serial.println(F("****"));
  if (!fona.HTTP_POST_start(POST_URL, F("application/json"), (uint8_t *) datapoint_c_str, strlen(datapoint_c_str), &statuscode, (uint16_t *)&length)) {
    Serial.println("Failed!");
  }
  while (length > 0) {
    while (fona.available()) {
      char c = fona.read();

      Serial.write(c);

      length--;
      if (! length) break;
    }
  }
  Serial.println("Debug Step 9");
  Serial.println(F("\n****"));
  fona.HTTP_POST_end();
  Serial.println("Debug Step 10");
  while(1){//Testing step to allow me to verify that any of this actually works.
    delay(16000);
  }


}
