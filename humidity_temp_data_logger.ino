/*
   [tmep1 , temp2 , temp3 , temp3 , temp4 , temp5 , 34dth_h , 34dth_t, 34dth_feels , 35dth_h , 35dth_t , 35dth_feels]
   dth 34 inside
   dth35 outside
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Time.h>
#include <OneWireNg.h>
#include <DallasTemperature.h>
#include "DHT.h"

#define DHTPIN 19
#define DHTPIN2 18

#define DHTTYPE DHT11

#define FILESYSTEM SPIFFS
#define FORMAT_FILESYSTEM false
#define DBG_OUTPUT_PORT Serial

#if FILESYSTEM == FFat
#include <FFat.h>
#endif
#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif

const char* ssid = "data logger";
const char* password = "12345678";
const char* host = "esp32";
const int oneWireBus = 4;

int i, k;
String data;
float reading;
unsigned long prev_mili;
int interval = 30000; //---------sampling rate, time

WebServer server(80);
ESP32Time rtc;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);
File fsUploadFile;


String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool exists(String path) {
  bool yes = false;
  File file = FILESYSTEM.open(path, "r");
  if (!file.isDirectory()) {
    yes = true;
  }
  file.close();
  return yes;
}

bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "data.txt";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(pathWithGz) || exists(path)) {
    if (exists(pathWithGz)) {
      path += ".gz";
    }
    File file = FILESYSTEM.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void setup(void) {

  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);

  sensors.begin();
  dht.begin();
  dht2.begin();

  if (FORMAT_FILESYSTEM) FILESYSTEM.format();
  FILESYSTEM.begin();
  {
    File root = FILESYSTEM.open("/");
    File file = root.openNextFile();
    while (file) {
      String fileName = file.name();
      size_t fileSize = file.size();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      file = root.openNextFile();
    }
    DBG_OUTPUT_PORT.printf("\n");
  }


  //WIFI INIT
  if (digitalRead(23) == HIGH) {
    DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
    if (String(WiFi.SSID()) != String(ssid)) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ssid, password);
    }
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("Connected! IP address: ");
    DBG_OUTPUT_PORT.println(WiFi.localIP());

    MDNS.begin(host);
    DBG_OUTPUT_PORT.print("Open http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local to see the file browser");

    server.on("/", HTTP_GET, []() {
      if (!handleFileRead("/data.txt")) {
        server.send(404, "text/plain", "FileNotFound");
      }
    });
    server.onNotFound([]() {
      if (!handleFileRead(server.uri())) {
        server.send(404, "text/plain", "FileNotFound");
      }
    });
    server.begin();
  }
  //  for (int k = 0; k < 100; k++) {
  //    for (int i = 0; i < 5; i++) {
  //      reading += analogRead(pins[i]);
  //    }
  //  }
  //  reading = reading / 500;
  //  for (int i = 0; i < 5; i++) {
  //    offsets[i] = reading - analogRead(pins[i]);
  //  }
  //get heap status, analog input value and all GPIO statuses in one json call
}

//float v2temp(float Vo) {
//  R2 = R1 * ((4095 / Vo) - 1.0);
//  logR2 = log(R2);
//  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
//  T = T - 273.15;
//  return T;
//}

void loop(void) {
  server.handleClient();
  sensors.requestTemperatures();
  if (digitalRead(23) == LOW) {
    if ( millis() - prev_mili >= interval) {
      DBG_OUTPUT_PORT.print("doing\n");
      prev_mili = millis();
      File values = FILESYSTEM.open("/data.txt", "a");
      data = rtc.getTime() + ",";
      for (int i = 0; i < 5; i++) {
        // data += String(v2temp(reading), 1) + "," ;
        data += String(sensors.getTempCByIndex(i)) + "," ;
      }
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      float h2 = dht2.readHumidity();
      float t2 = dht2.readTemperature();
      data += String(h) + "," ;
      data += String(t) + "," ;
      data += String(h2) + "," ;
      data += String(t2) + "," ;
      data += "\n" ;
      const uint8_t* buffer = (const uint8_t*)data.c_str();
      size_t size = data.length();

      values.write(buffer, size);

      values.close();
    }
  }
}
