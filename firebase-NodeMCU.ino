#include <Arduino.h>
#if defined ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#elif defined ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#error Wrong platform
#endif 
#include <WifiLocation.h>

#define GOOGLE_KEY "" // Clave API Google Geolocation
#define SSID "" // SSID de tu red WiFi
#define PASSWD "" // Clave de tu red WiFi
#define HOSTFIREBASE "" // Host o url de Firebase
#define LOC_PRECISION 7 // Precisión de latitud y longitud
#define FINGERPRINT "56 F9 EC 8D 3C 71 B8 74 57 83 DC 05 EE 02 9A 58 41 9A 3D 69"

// Llamada a la API de Google
WifiLocation location(GOOGLE_KEY);
location_t loc; // Estructura de datos que devuelve la librería WifiLocation

// Variables
byte mac[6];
String macStr = "";
String nombreComun = "NodeMCU";

// Cliente WiFi
WiFiClientSecure client;


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
  client.setInsecure();
  // Conexión con la red WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(SSID);
    // Connect to WPA/WPA2 network:
    WiFi.begin(SSID, PASSWD);
    // wait 5 seconds for connection:
    delay(5000);
    Serial.print("Status = ");
    Serial.println(WiFi.status());
  }
  setClock ();
  // Obtenemos la MAC como cadena de texto
  macStr = obtenerMac();

  Serial.print("MAC NodeMCU: ");
  Serial.println(macStr);
}

void loop() {
  setClock ();
  // Obtenemos la geolocalización WiFi
  loc = location.getGeoFromWiFi();

  // Mostramos la información en el monitor serie
  Serial.println("Location request data");
  Serial.println(location.getSurroundingWiFiJson());
  Serial.println("Latitude: " + String(loc.lat, 7));
  Serial.println("Longitude: " + String(loc.lon, 7));
  Serial.println("Accuracy: " + String(loc.accuracy));

  // Hacemos la petición HTTP mediante el método PUT
  peticionPut();

  // Esperamos 15 segundos
  delay(15000);
}

/********** FUNCIÓN PARA OBTENER MAC COMO STRING **********/
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

/********** FUNCIÓN QUE REALIZA LA PETICIÓN PUT **************/
void peticionPut()
{
  // Cerramos cualquier conexión antes de enviar una nueva petición
  client.stop();
  client.flush();
  /* Bloque para validar certificados.
  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }*/
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
    payload += String(loc.lat, LOC_PRECISION);
    payload += ",";
    payload += "\"lon\":";
    payload += String(loc.lon, LOC_PRECISION);
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
