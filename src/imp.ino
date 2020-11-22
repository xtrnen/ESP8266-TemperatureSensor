/**
 * Jan Trneny
 * xtrnen03
 * datum posledni zmeny: 16.12.2018
 * inspirace a pouzite knihovny: http://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html
 */
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h>

ESP8266WebServer server(80);  //Instance serveru na portu 80S
IPAddress IP(192, 168, 1, 42); //IP adresa
IPAddress MASK(255, 255, 255, 0); //Maska
DNSServer myDNS;  //Instance DNS
const byte DNS_PORT = 53; //Port DNS - zakladni port

OneWire oneWire(D1);

DallasTemperature sensor(&oneWire);

int overflowOffset = 0; //kolikrat pretekl
int TIMER = 60; //60s
unsigned int count = 0; //pocet zaznamu
unsigned int ms = 0;  //minut kolik bylo snímáno

/*Zasilani dat na server*/
void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if(!file){
    Serial.println("Error: Open index.html");
  }
  size_t sent = server.streamFile(file, "text/html"); //zaslani indexu clientu
  file.close();
}

/*Nastaveni serveru*/
void setupServer() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IP, IP, MASK);  //Konfigurace serveru IP,Gateway stejna
  WiFi.softAP("xtrnen03-IMP"); //Nazev access pointu

  IPAddress myIP = WiFi.softAPIP();

  myDNS.setTTL(300);  //TTL
  myDNS.setErrorReplyCode(DNSReplyCode::ServerFailure);
  myDNS.start(DNS_PORT, "www.xtrnen03.vutbr.cz", IP); //Spusteni DNS

  server.onNotFound([]() {
    server.send(200, "text/html", "<h1>404 not found!</h1>");
  });
  
  server.on("/", handleRoot); //Nastaveni uvodni strany
  
  server.on("/main", [](){
    server.send(200, "text/plain", String(sensor.getTempCByIndex(0) - 2));  //zaslani aktualni teploty
  });

  server.on("/csv", [](){
    File file = SPIFFS.open("/temps.txt", "r");
    size_t sent = server.streamFile(file, "text/plain");  //zaslani statistik
    file.close();
  });
  
  server.on("/w3.css", [](){
    File file = SPIFFS.open("/w3.css", "r");
    size_t sent = server.streamFile(file, "text/css");  //zaslani css pro html
    file.close();
  });
  
  server.begin(); //Spusteni serveru
}

/*Ziskani teploty*/
void getTemperature() {
  sensor.requestTemperatures();
  Serial.print("Temperature: ");
  Serial.println(sensor.getTempCByIndex(0) - 2);
  saveData(); //zapis do statistik
}

/*Ulozeni do JSON*/
void saveData() {
  File file = SPIFFS.open("/temps.txt", "a+");  //otevreni souboru pro zapis a+ -> append
  if(!file){
    Serial.println("Error: Open temps.txt");
  }

  file.print(count);  //zapis minuty od zacatku
  file.print(",");  //oddeleni polozek
  file.print(sensor.getTempCByIndex(0) - 2);  //zapis teploty
  file.print(";");  //oddeleni zaznamu
  
  file.close();
}

void setup() {
  SPIFFS.begin(); //spusteni file systemu
  Serial.begin(115200);
  setupServer();  //nastaveni serveru a jeho spusteni
  sensor.begin(); //zapnuti senzoru
  /*Kontrola existence souboru - DEBUG*/
  if(SPIFFS.exists("/temps.txt")){
    Serial.println("File temps.txt exists in FS");
  }
  if(SPIFFS.exists("/index.html")){
    Serial.println("File index.html exists in FS");
  }
  if(SPIFFS.exists("/w3.css")){
    Serial.println("File w3s.css exists in FS");
  }
}

void loop() {
  myDNS.processNextRequest();
  server.handleClient();
  unsigned int milis = millis();
  if(milis < ms){
    //overflow
    ms = milis;
    overflowOffset++;
  }
  else{
    if((milis % 60000) == 0){
      //kazdou minutu ziskat hodnotu
      count++;
      getTemperature();
    }
  }
}
