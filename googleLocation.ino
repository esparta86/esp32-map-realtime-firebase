
#include <Arduino.h>
#if defined ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#elif defined ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#error Wrong platform
#endif

#include <WifiLocation.h>

//NEW
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


#define DATABASE_URL "https://wifi-arduino-esp32.firebaseio.com/" 

const char* googleApiKey = "";
const char* firebaseApiKey = "";
const char* ssid = "";
const char* passwd = "";
//const char* HOSTFIREBASE = "wifi-arduino-esp32.firebaseio.com";
const char* HOSTFIREBASE = "";

//unsigned char* LOC_PRECISION =7;
WifiLocation location(googleApiKey);
location_t loc; // data format structure retrieved from wofilocation library


// Variables
byte mac[6];
String macStr = "";
String nombreComun = "ARDUINO ESP32";


// Cliente WiFi
WiFiClientSecure client;


// NEW

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

String timenow;
FirebaseJson json;
FirebaseJson json2;
FirebaseData fbdo2;

const long  gmtOffset_sec = -21600;
const int   daylightOffset_sec = 0;
const char* ntpServer = "pool.ntp.org";

// Set time via NTP, as required for x.509 validation
String setClock () {
    //configTime (0, 0, "pool.ntp.org", "time.nist.gov");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    Serial.print ("Waiting for NTP time sync: ");
    time_t now = time (nullptr);
    while (now < 8 * 3600 * 2) {
        delay (500);
        Serial.print (".");
        now = time (nullptr);
    }
    struct tm timeinfo;
    gmtime_r (&now, &timeinfo);
    Serial.print ("\n");
    Serial.print ("Current time: ");
    Serial.print (asctime (&timeinfo));

    return (String)""+time(&now);
}


void setup() {
  Serial.begin(115200);
  // Connect to WPA/WPA2 network
  //WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // wait 5 seconds for connection:
    WiFi.begin(ssid, passwd);
    delay(5000);
    Serial.print("Status = ");
    Serial.println(WiFi.status());

  }
  Serial.println ("Connected");
  setClock ();
  location_t loc2 = location.getGeoFromWiFi();
 
  Serial.println("Location request data");
  Serial.println(location.getSurroundingWiFiJson());
  Serial.println("Latitude: " + String(loc2.lat, 7));
  Serial.println("Longitude: " + String(loc2.lon, 7));
  Serial.println("Accuracy: " + String(loc2.accuracy));

  // Obtenemos la MAC como cadena de texto
  macStr = obtenerMac();
 
  Serial.print("MAC ESP32: ");
  Serial.println(macStr);

  /* Assign the api key (required) */
  config.api_key = firebaseApiKey;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

    /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}

void loop() {

  
 // peticionPut();

  delay(15000);
 

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

     timenow = setClock ();
     loc = location.getGeoFromWiFi();

     Serial.println("Location request data");
     Serial.println(location.getSurroundingWiFiJson());
     Serial.println("Latitude: " + String(loc.lat, 7));
     Serial.println("Longitude: " + String(loc.lon, 7));
     Serial.println("Accuracy: " + String(loc.accuracy));

     
    //json2.add("child_of_002",123.56);
    json.add("Latitud",String(loc.lat, 7));
    json.add("Longitude",String(loc.lon, 7));
    json.add("Accuracy",String(loc.accuracy, 7));
    json.add("timestamp",timenow);

    String keyPath = "/maps/"+macStr+"/";
    keyPath +=timenow;
    

    if (Firebase.RTDB.pushJSON(&fbdo2, keyPath, &json)) {
      Serial.println(fbdo2.dataPath());
      Serial.println(fbdo2.pushName());
      Serial.println(fbdo2.dataPath() + "/"+ fbdo2.pushName());
      
    }else{
      Serial.println(fbdo2.errorReason());
    }

    
  }

}





String obtenerMac()
{
  // Obtenemos la MAC del dispositivo
  WiFi.macAddress(mac);

  // Convertimos la MAC a String
  String keyMac = "";
  for (int i = 0; i < 6; i++)
  {
    String pos = String((uint8_t)mac[i], HEX);
    if (mac[i] <= 0xF)
      pos = "0" + pos;
    pos.toUpperCase();
    keyMac += pos;
    if (i < 5)
      keyMac += ":";
  }

  // Devolvemos la MAC en String
  return keyMac;
}


