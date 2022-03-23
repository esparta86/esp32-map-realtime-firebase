
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

const char* googleApiKey = "123AIzaSyBz-ov1S4NJZxZ-0WXf0l89OyfF1xmBB9A45";
const char* firebaseApiKey = "123AIzaSyBwL-A3smdyKK5eGP4YkCdsaAsYP3Vx42k45";
const char* ssid = "NIDAIME";
const char* passwd = "";
//const char* HOSTFIREBASE = "wifi-arduino-esp32.firebaseio.com";
const char* HOSTFIREBASE = "wifi-arduino-esp32-default-rtdb.firebaseio.com";

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



// Set time via NTP, as required for x.509 validation
void setClock () {
    configTime (0, 0, "pool.ntp.org", "time.nist.gov");

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
  macStr = getMac();
 
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

  /*setClock ();
  loc = location.getGeoFromWiFi();

  Serial.println("Location request data");
  Serial.println(location.getSurroundingWiFiJson());
  Serial.println("Latitude: " + String(loc.lat, 7));
  Serial.println("Longitude: " + String(loc.lon, 7));
  Serial.println("Accuracy: " + String(loc.accuracy));

  peticionPut();

  delay(15000);
 */

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, "test/int", count)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    count++;
    
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "test/float", 0.01 + random(0,100))){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }

}


String getMac()
{
  WiFi.macAddress(mac);

  String keyMac = "";

  for (int i = 0; i < 6; i++)
  {
    String pos = String((uint8_t)mac[i], HEX);
    if (mac[i] <= 0xF)
      pos = "0" + pos;
    pos.toUpperCase();
    keyMac += ":";
  }

  return keyMac;
}


void peticionPut()
{
  // Cerramos cualquier conexión antes de enviar una nueva petición
  client.stop();
  client.flush();
  // Enviamos una petición por SSL
    
  if (client.connect(HOSTFIREBASE, 443)) {
    // Petición PUT JSON
    String toSend = "PUT /dispositivo/";
    toSend += macStr;
    toSend += ".json HTTP/1.1\r\n";
    toSend += "Host:";
    toSend += HOSTFIREBASE;
    toSend += "\r\n" ;
    toSend += "Content-Type: application/json\r\n";
    String payload = "{\"lat\":";
    payload += String(loc.lat, 7);
    payload += ",";
    payload += "\"lon\":";
    payload += String(loc.lon, 7);
    payload += ",";
    payload += "\"prec\":";
    payload += String(loc.accuracy);
    payload += ",";
    payload += "\"nombre\": \"";
    payload += nombreComun;
    payload += "\"}";
    payload += "\r\n";
    toSend += "Content-Length: " + String(payload.length()) + "\r\n";
    toSend += "\r\n";
    toSend += payload;
    Serial.println(toSend);
    client.println(toSend);
    client.println();
    client.flush();
    client.stop();
    Serial.println("Todo OK");
  } else {
    // Si no podemos conectar
    client.flush();
    client.stop();
    Serial.println("Algo ha ido mal");
  }
}
