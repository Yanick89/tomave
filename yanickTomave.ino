/*  ********

   K6K K'PO  esp version 1.5

    ********
*/
// insertion des bibiotheque
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <DHT.h>
#include <Hash.h>
#include "DHT.h"

// information de connexion
const char* ssid = "CANALBOX-BC98";
const char* password = "4730708947";

//definir pin 2 comme pin de connection du capteur 
#define DHTPIN D5  // pin Digital D5 connecte au capteur DHT sensor

// type de capteur utilise:
#define DHTTYPE    DHT11     
DHT dht(DHTPIN, DHTTYPE);


// declaration des variables 

  unsigned long tempsInitial1=0;
  unsigned long tempsPrecedent1=0;
  bool etatLed1=LOW;
  const int led0=D0;

float t = 0.0; // variable temperature
float h = 0.0; // variable humiditer

//const int systeme = D5;//initialisation du systeme(pas encore pret)

//lampe(source de chaleur)
const int pin1 = D1;// Assignation de la led a la pin D1
String etat1;//  Stockage de l'etat de la led 1

//ventilateur(regulateur de temperature)
const int pin2 = D2;// Assignation de la led a la pin D2
String etat2; //  Stockage de l'etat de la led 2

//moteur (basculement des oeufs)
const int pin3 = D3;// Assignation de la led a la pin D3
String etat3; // Stockage de l'etat de la led 3
int etat3_3 = HIGH;

const long OnDuree = 25000;// temps d'allumage de la LED
const long OffDuree = 10000;// temps d'arret de la LED

unsigned long temps = 0;//ceci est utilisé par le code il commmence a compter a partir du moment ou la carte est sous-tension.
// utilisation de "unsigned long" pour les variables qui contiennent du temps
// La valeur deviendra rapidement trop grande pour un "int" à stocker
unsigned long tempsPrecedent = 0; // stockera la dernière fois que DHT a été mis à jour
// Met à jour les lectures DHT toutes les 10 secondes
const long interval = 2000;

// creer un server web asynchrone. communication par le port 80 
AsyncWebServer server(80);

// afficher l'etat de la valeur de la LED dans l'espace reserver (sur la page web)
String processor(const String& var) {
  Serial.println(var);
  if (var == "STATE1") {
    if (digitalRead(pin1)) {
      etat1 = "ON";
    }
    else {
      etat1 = "OFF";
    }
    Serial.print(etat1);
    return etat1;
  }

  if (var == "STATE2") {
    if (digitalRead(pin2)) {
      etat2 = "ON";
    }
    else {
      etat2 = "OFF";
    }
    Serial.print(etat2);
    return etat2;
  }

  if (var == "STATE3") {
    if (digitalRead(pin3)) {
      etat3 = "ON";
    }
    else {
      etat3 = "OFF";
    }
    Serial.print(etat3);
    return etat3;
  }

  //Serial.println(var);
  if (var == "TEMPERATURE") {
    return String(t);
  }
  else if (var == "HUMIDITY") {
    return String(h);
  }
  return String();

}
   

void setup() {
  // declaration du tu type Port série
  Serial.begin(115200);
  pinMode(led0,OUTPUT);
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  pinMode(pin3, OUTPUT);

  // Initialisation du capteur
  dht.begin();// initialisation pour DHT11

  // Initialisation SPIFFS (Serial Peripheral Interface Flash File System)
  if (!SPIFFS.begin()) {
    Serial.println("Une erreur s'est produite lors du montage de SPIFFS");
    return;
  }

  // connexion au wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connexion au Wi-Fi..");
  }

  // Imprimer l'adresse IP locale ESP8266
  Serial.println(WiFi.localIP());
  
// chemin vers la racine / page web
  server.on("/tomave", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/tomave.png", "image/png");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // chemin vers le fichier style.css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // chemin vers la pin GPIO définit sur etat haut
  server.on("/on1", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(pin1, HIGH);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // chemin vers la pin GPIO définit sur etat bas
  server.on("/off1", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(pin1, LOW);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // chemin vers la pin GPIO définit sur etat haut
  server.on("/on2", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(pin2, HIGH);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

 // chemin vers la pin GPIO définit sur etat bas
  server.on("/off2", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(pin2, LOW);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // chemin vers la pin GPIO définit sur etat haut
  server.on("/on3", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(pin3, HIGH);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // chemin vers la pin GPIO définit sur etat bas
  server.on("/off3", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(pin3, LOW);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(h).c_str());
  });

  // Démarrer le serveur
  server.begin();
}

//declaration des fonctions

void processus_incubateur(float tmp_min, float tmp_max)
{
  //int etat3_3=etat3.toInt();

  unsigned long temps0 = millis();
  unsigned long tempsActuel = millis();
  if (tempsActuel - tempsPrecedent >= interval) {

    // enregistre la dernière mis à jour les valeurs DHT
    tempsPrecedent = tempsActuel;

   // Lecture de la température en Celsius (valeur par défaut)
    float newT = dht.readTemperature();
    //Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);

    // si la lecture de la température a échoué, ne change pas la valeur t
    if (isnan(newT)) {
      Serial.println("Échec de la lecture du capteur DHT !");
    }
    else {
      t = newT;
      Serial.println(t);
    }
    // lecture Humidite
    float newH = dht.readHumidity();
     // si la lecture l'humidite a échoué, ne change pas la valeur h
    if (isnan(newH)) {
      Serial.println("Échec de la lecture du capteur DHT !");
    }
    else {
      h = newH;
      Serial.println(h);
    }
  }

  if (t >= tmp_min && t < tmp_max) // (t >= 10 && t < 32) code de bqse
  {
    digitalWrite(pin1, HIGH);

  }
  if (t >= tmp_max) // (t >= 32) code de bqse
  {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, HIGH);
    delay(10000);
  } else {
    digitalWrite(pin2, LOW);
  }



  if ( etat3_3 == HIGH )
  {
    if ( (temps0 - temps) >= OnDuree) {
      etat3_3 = LOW;// changer l'état de la LED en la mettant a l'etat bas
      //digitalWrite(pin3,etat3_3); // allume ou éteint la LED
      temps = temps0 ; // sauvegarde l'heure du decompte l'etat bas
    }
  }
  else
  {
    if ( (temps0 - temps) >= OffDuree) {
      etat3_3 = HIGH; // changer l'état de la LED en la mettant a l'etat haut
      //digitalWrite(pin3,etat3_3); // allume ou éteint la LED
      temps = temps0 ; // // sauvegarde l'heure du decompte l'etat haut
    }
  }

  digitalWrite(pin3, etat3_3); // allume ou éteint la LED
}


void loop() {

    tempsInitial1=millis();
   if((tempsInitial1-tempsPrecedent1)>100){
    tempsPrecedent1=tempsInitial1;
    etatLed1=!etatLed1;
    digitalWrite(led0,!etatLed1);
    //Serial.print(F("LED State : "));Serial.println(etatLed1);
  }
  
  int test_local=0; //variable de test local
  

  switch(test_local)
  {
    case 1:processus_incubateur(10,32); // incubateur poule avec les parametre pour poule
    break;
  }
  
  
 
  
}
