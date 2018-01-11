#include "Adafruit_FONA.h"
#include <string.h>

#define FONA_RX 10
#define FONA_TX 9
#define FONA_RST 6

#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

void setup() {

  while (! Serial);

  Serial.begin(115200);
  Serial.println(F("Adafruit FONA 3G module"));
  Serial.println(F("Initializing FONA... (May take a few seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while(1);
  }
  Serial.println(F("FONA is OK"));
  char imei[16] = {0}; 
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("SIM card IMEI: "); Serial.println(imei);
  }

  fonaSerial->print("AT+CNMI=2,1\r\n");  //set up the FONA to send a +CMTI notification when an SMS is received

  Serial.println("FONA Ready");
 

  Serial.println(F("Enabling GPS..."));
  fona.enableGPS(true);
}

char fonaNotificationBuffer[64];          
char smsBuffer[250];

const String mobile [] = {"+1......","+1.......","+1......."}; // Add the mobile numbers

void loop() {
  
  delay(2000);
  char* bufPtr = fonaNotificationBuffer;    
  float latitude, longitude, speed_kph, heading, speed_mph, altitude;
  //Add a terminal NULL to the notification string
  *bufPtr = 0;
  int slot = 0;           
  int charCount = 0;
  
  boolean gps_success = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);
  
  String gps_location = "https://www.google.com/maps/search/?api=1&query=";
  char str1[20]="";
  char str2[20]="";
  char *location="";
  
  do  {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaNotificationBuffer)-1)));

  //Scan the notification string for an SMS received notification.
  //  If it's an SMS message, we'll get the slot number in 'slot'
  if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)||(fona.available()&& gps_success)) {
  Serial.print("slot: "); Serial.println(slot);

  dtostrf(latitude,7, 4, str1);
  gps_location += str1;
  dtostrf(longitude,7, 4, str2);
  gps_location += ",";
  gps_location += str2;
  Serial.print("GPS location:");
  Serial.println(gps_location);

  location = gps_location.c_str();
  
  char callerIDbuffer[32] = " ";  //we'll store the SMS sender number in here
  
  // Retrieve SMS sender address/phone number.
  if (!fona.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
  
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);

      // Retrieve SMS value.
        uint16_t smslen;
        if (fona.readSMS(slot, smsBuffer, 250, &smslen)) { // pass in buffer and max len!
          Serial.println(smsBuffer);
        }
  String str(callerIDbuffer);
  for(int i =0;i<4;i++){
    if(str.equals(mobile[i])){   
         
        //Send back an automatic response
        Serial.println("Sending response...");
        if (!fona.sendSMS(callerIDbuffer,location)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("Sent!"));
        }
        // delete the original msg after it is processed
        //   otherwise, we will fill up all the slots
        //   and then we won't be able to receive SMS anymore
        if (fona.deleteSMS(slot)) {
          Serial.println(F("OK!"));
        } else {
          Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
          fona.print(F("AT+CMGD=?\r\n"));
        }
      }
      else {
      Serial.println("Mobile number is not Trusted");
     }
    }
  }
}

