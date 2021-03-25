#include <DallasTemperature.h>
#include <SPI.h>
#include <"DHT.h">
#include <"time.h">
#include <OneWire.h>
#include <PubSubClient.h>
#include <WiFiNINA.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <RTCZero.h>

/*Token mqtt*/
#define TOKEN 

/*sensor humedad ambiente y temperatura*/
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#define DHTPIN1 0
#define DHTPIN2 1


// Initialize wifi

WiFiClient mkr1010Client;
PubSubClient client(mkr1010Client);
long lastData = 0;
RTCZero rtc;

const char* ssid = "lezica";
const char* password = "papicha4132";
const char mqtt_server[] = "192.168.0.103"; 
const char publishTopic[] = "esp32/satelitegrow"; 
int status = WL_IDLE_STATUS;      //connection status
WiFiServer server(80);            //server socket




const int GMT = 21; //change this to adapt it to your time zone
inthours;
int Minutes;
int Seconds;


// Initialize DHT sensor.

DHT sensor_dht1(DHTPIN1, DHT22);
DHT sensor2_dht2(DHTPIN2, DHT22);


// Variables DHT sensor.

float sensordht1_Temperature;
float sensordht1_Humidity;
float sensordht2_Temperature;
float sensordht2_Humidity;
float Temperature = 0;
float Temperature2 = 0;
float Humidity = 0;
float Humidity2 = 0;

// Variables tiempo
Stringhours;
String minutes;
String seconds;
String days;




// Pin donde se conectan los sensores de temperatura de agua
const int Sensor_Water_Temperature_pin = 3;

// Instancia a las clases OneWire y DallasTemperature, sensores de temperatura de agua
OneWire oneWireObjeto(Sensor_Water_Temperature_pin);
DallasTemperature sensorDS18B20(&oneWireObjeto);


/*sensor humedad tierra*/
const float PIN_Sensor_Humedad_1a = A0;
const float PIN_Sensor_Humedad_1b = A1;
const float PIN_Sensor_Humedad_2a = A2;
const float PIN_Sensor_Humedad_2b = A3;


/*Variables para guardar datos de sensores de nivel de agua bandejas*/

const float Pin_Water_tray_level_sensor1 = A4;
const float Pin_Water_tray_level_sensor2 = A5;


/*Variables para guardar datos de sensores de humedad*/


float  Soil_Humidity_Sensor_1a = 0;
float  Soil_Humidity_Sensor_1b = 0;
float  Soil_Humidity_Sensor_2a = 0;
float  Soil_Humidity_Sensor_2b = 0;
float  Water_tray_level_sensor1 =0;
float  Water_tray_level_sensor2 = 0;


/*Electrovalvulas*/

const int Solenoid_Valvule_1 = 7;
const int Solenoid_Valvule_2  = 8;
const int Solenoid_Valvule_3  = 9;
const int Solenoid_Valvule_4  = 10;

/*Bombas de agua*/
const int waterpump_soil = 6;
const int waterpump_hydro = 11;

float  WaterTemp_soil = 0;
float  WaterTemp_Hydro = 0;




void setup_wifi(){

  delay(10);
  Serial.println();
  Serial.print(Connecting to );
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while( WiFi.status() != WL_CONNECTED){

    delay(500);
    Serial.print(.);   
  }

  randomSeed(micros());
  Serial.println();
  Serial.println(WiFi connected);
  Serial.println(IP address: );
  Serial.println(WiFi.localIP());
 
}


void reconnect() {

  while (!client.connected()) {

    Serial.print(Attempting MQTT connection ....);



    if (client.connect(ClientID, TOKEN, NULL)) {

      Serial.println(Connected to MQTT Broker);
      digitalWrite(LED_BUILTIN, HIGH);
    }

    else
    {
      Serial.print(failed, rc=);
      Serial.print(client.state());
      Serial.println(try again in 5 second);
      digitalWrite(LED_BUILTIN, LOW);
      delay(5000);
    }


  }

}


void setup() {

  // Iniciamos la comunicación serie
  Serial.begin(115200);
  Wire.begin(); //(SDA, SCL);
  setup_wifi();
  client.setServer(mqtt_server, 1883);


  rtc.begin();


 if (!client.connected()) {
    reconnect();
  }



  if ( WiFi.status() ==  WL_CONNECTED ) 
  {
    // WiFi is UP,  do what ever
  } else
  {

  // wifi down, reconnect here
   WiFi.begin( ssid, password );
    int WLcount = 0;
    int UpCount = 0;
    while (WiFi.status() != WL_CONNECTED && WLcount < 200 ) 
    {
      delay( 100 );
         Serial.print(.);
         if (UpCount >= 60)  // just keep terminal from scrolling sideways
         {
          
           UpCount = 0;
               Serial.print(\n);
         }
         ++UpCount;
      ++WLcount;
    }
  }

  

  


  

  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime();
    numberOfTries++;
  }
  while ((epoch == 0) && (numberOfTries < maxTries));

  if (numberOfTries == maxTries) {
    Serial.print(NTP unreachable!!);
    while (1);
  }
  else {
    Serial.print(Epoch received: );
    Serial.println(epoch);
    rtc.setEpoch(epoch);

    Serial.println();
  }



  // Iniciamos el bus 1-Wire
  sensorDS18B20.begin();

  // sensors humedad ambiente
  sensor_dht1.begin();
  sensor2_dht2.begin();

  // electrovalvulas ca
  pinMode(Solenoid_Valvule_1, OUTPUT);
  pinMode(Solenoid_Valvule_2, OUTPUT);
  pinMode(Solenoid_Valvule_3, OUTPUT);
  pinMode(Solenoid_Valvule_4, OUTPUT);
  pinMode(waterpump_soil, OUTPUT);
  pinMode(waterpump_hydro, OUTPUT);


  // electrovalvulas apagadas
  digitalWrite(Solenoid_Valvule_1,  HIGH);
  digitalWrite(Solenoid_Valvule_2,  HIGH);
  digitalWrite(Solenoid_Valvule_3,  HIGH);
  digitalWrite(Solenoid_Valvule_4,  HIGH);

  // bomba apagada
  digitalWrite(waterpump_soil, HIGH);
  digitalWrite(waterpump_hydro, HIGH);




}

void loop() {


  if (!client.connected()) {
    reconnect();

  }

  //    if (client) {
  //      printWEB();
  //    }




  printDate();
  printTime();
  Serial.println();


  int hours = (rtc.gehours() + GMT);
  int Minutes = (rtc.getMinutes());
  int Seconds = (rtc.getSeconds());






  // Leemos y mostramos los datos de los sensores de temperatura de agua DS18B20

  sensorDS18B20.requestTemperatures();
  Serial.print( Water_Temperature_Microgreens: );
  Serial.print(sensorDS18B20.getTempCByIndex(0));
  WaterTemp_soil  = sensorDS18B20.getTempCByIndex(0);
  Serial.print(C);
  Serial.print(\t);
  Serial.print(Water_Temperature_hydropònics: );
  Serial.print(sensorDS18B20.getTempCByIndex(1));
  WaterTemp_Hydro = sensorDS18B20.getTempCByIndex(1);
                    Serial.print( C);
  Serial.print(\n);


  // obtenemos valores de temperatura y humedad y guardamos variables

  Temperature = sensor_dht1.readTemperature();
  Serial.print(Temperatura ambiente: );
  Serial.print (Temperature);
  Serial.print(\t);
  // onbtenemos valores de humedad
  Humidity = sensor_dht1.readHumidity();
  Serial.print(humedad ambiente: );
  Serial.print ( Humidity);

  Serial.print(\n);


  // obtenemos valores de temperatura2 y humedad2 y guardamos variables

  Temperature2 = sensor2_dht2.readTemperature(1);
  Serial.print(Temperatura ambiente2: );
  Serial.print (Temperature2);
  Serial.print(\t);
  // onbtenemos valores de humedad
  Humidity2 = sensor2_dht2.readHumidity(1);
  Serial.print(humedad ambiente2: );
  Serial.print ( Humidity2);

  Serial.print(\n);

  // leemos sensor1 y lo guardamos en una variable

  Soil_Humidity_Sensor_1a = analogRead(PIN_Sensor_Humedad_1a);
  Serial.print(Soil_Humidity_Sensor_1a: );
  Serial.print (Soil_Humidity_Sensor_1a);
  Serial.print(\t);

  // leemos sensor2 y lo guardamos en una variable

  Soil_Humidity_Sensor_1b = analogRead(PIN_Sensor_Humedad_1b);
  Serial.print(Soil_Humidity_Sensor_1b: );
  Serial.print (Soil_Humidity_Sensor_1b);
  Serial.print(\t);


  // leemos sensor3 y lo guardamos en una variable
  Soil_Humidity_Sensor_2a = analogRead(PIN_Sensor_Humedad_2a);
  Serial.print(Soil_Humidity_Sensor_2a: );
  Serial.print (  Soil_Humidity_Sensor_2a);
  Serial.print(\t);



  // leemos sensor4 y lo guardamos en una variable

  Soil_Humidity_Sensor_2b = analogRead(PIN_Sensor_Humedad_2b);
  Serial.print(Soil_Humidity_Sensor_2b: );
  Serial.print (Soil_Humidity_Sensor_2b);
  Serial.print(\t);

  // leemos sensor de nivel de agua_1 bandejas y lo guardamos en una variable

  Water_tray_level_sensor1 = analogRead(Pin_Water_tray_level_sensor1);
  Serial.print( Water_tray_level_sensor1 : );
  Serial.print ( Water_tray_level_sensor1 );
  Serial.print(\t);

  // leemos sensor de nivel de agua_2 bandejas y lo guardamos en una variable

  Water_tray_level_sensor2 = analogRead( Pin_Water_tray_level_sensor2 );
  Serial.print( Water_tray_level_sensor2 : );
  Serial.print ( Water_tray_level_sensor2);
  Serial.print(\t);


  Serial.print(\n);


  //****** Riego Tierra********//

  if (hours) ==  (10) && (minutes) == (00) && (seconds) == (00)) {
    Serial.println(Check value SHT1);
    check_Soil_Humidity_Sensor_1a();
  }



  if (hours) ==  (10) && (minutes) == (01) && (seconds) == (00)) {
    Serial.println(Check value SHT2);
    check_Soil_Humidity_Sensor_1b();
  }




  if (hours) ==  (10) && (minutes) == (02) && (seconds) == (00)) {
    Serial.println(Check value SHT3);

    check_Soil_Humidity_Sensor_2a();
  }



  if (hours) ==  (10) && (minutes) == (03) && (seconds) == (00)) {
    Serial.println(Check value SHT4);

    check_Soil_Humidity_Sensor_2b();
  }


  if (hours) ==  (19) && (minutes) == (00) && (seconds) == (00)) {
    Serial.println(Check value SHT1);
    check_Soil_Humidity_Sensor_1a();
  }



  if (hours) ==  (19) && (minutes) == (01) && (seconds) == (00)) {
    Serial.println(Check value SHT2);
    check_Soil_Humidity_Sensor_1b();
  }




  if (hours) ==  (19) && (minutes) == (02) && (seconds) == (00)) {
    Serial.println(Check value SHT3);

    check_Soil_Humidity_Sensor_2a();
  }



  if (hours) ==  (19) && (minutes) == (03) && (seconds) == (00)) {
    Serial.println(Check value SHT4);

    check_Soil_Humidity_Sensor_2b();
  }



  else {
    Serial.print(it's not time to check sensors);

  }

  //****** Riego Hidroponia********//


  // hacemos un riego a las 8 am

  if (hours) ==  (08) && (minutes) == (00)) {

    waterpumpon_function ();

  }
  else if (hours) == (08) && (minutes) == (05)) {

    waterpumpoff_function ();

  }

  if (hours) ==  (11) && (minutes) == (00)) {

    waterpumpon_function ();

  }
  else if (hours) == (11) && (minutes) == (05)) {

    waterpumpoff_function ();

  }

  if (hours) ==  (13) && (minutes) == (00)) {

    waterpumpon_function ();

  }
  else if (hours) == (13) && (minutes) == (05)) {

    waterpumpoff_function ();

  }

  if (hours) ==  (16) && (minutes) == (00)) {

    waterpumpon_function ();

  }
  else if (hours) == (16) && (minutes) == (05)) {

    waterpumpoff_function ();

  }

  if (hours) ==  (19) && (minutes) == (05)) {

    waterpumpon_function ();

  }
  else if (hours) == (19) && (minutes) == (10)) {

    waterpumpoff_function ();

  }

  if (hours) ==  (21) && (minutes) == (00)) {

    waterpumpon_function ();

  }
  else if (hours) == (21) && (minutes) == (05)) {

    waterpumpoff_function ();


  }
  if (hours) ==  (00) && (minutes) == (00)) {

    waterpumpon_function ();

  }
  else if (hours) == (00) && (minutes) == (10)) {

    waterpumpoff_function ();

  }

  if (hours) ==  (02) && (minutes) == (00)) {

    waterpumpon_function ();

  }
  else if (hours) == (02) && (minutes) == (10)) {

    waterpumpoff_function ();

  }


  else
  {

    Serial.print(hydroponics pump off);

  }





  // Convert the sensor values to string

  String Soil_humidity_1a  = String( Soil_Humidity_Sensor_1a );
  String Soil_humidity_1b  = String( Soil_Humidity_Sensor_1b );
  String Soil_humidity_2a  = String( Soil_Humidity_Sensor_2a );
  String Soil_humidity_2b  = String( Soil_Humidity_Sensor_2b );
  String Ambient_humidity1  = String(Humidity);
  String Ambient_humidity2  = String(Humidity2);
  String Ambient_Temperature1  = String(Temperature);
  String Ambient_Temperature2  = String(Temperature2);
  String Water_Temperature_Microgreens = String(WaterTemp_soil );
  String Water_Temperature_Hydroponics = String(WaterTemp_Hydro);
  String Water_level_Microgreens  = String(Water_tray_level_sensor1);
  String Water_level_Hydroponics  = String(Water_tray_level_sensor2);

  String payload = {\Sensors\:;

  payload += \Soil_humidity_1a\:;
  payload += Soil_humidity_1a;

  payload += \Soil_humidity_1b\:;
  payload += Soil_humidity_1b;

  payload += \Soil_humidity_2a\:;
  payload += Soil_humidity_2a;

  payload += \Soil_humidity_2b\:;
  payload += Soil_humidity_2b;

  payload += \Soil_humidity_2b\:;
  payload += Soil_humidity_2b;

  payload += \Ambient_humidity1\:;
  payload += Ambient_humidity1;

  payload += \Ambient_humidity2\:;
  payload += Ambient_humidity2;

  payload += \Ambient_Temperature1\:;
  payload += Ambient_Temperature1;

  payload += \Ambient_Temperature2\:;
  payload += Ambient_Temperature2;

  payload += \Water_Temperature_Microgreens\:;
  payload += Water_Temperature_Microgreens;

  payload += \Water_Temperature_Microgreens\:;
  payload += Water_Temperature_Microgreens;

  payload += \Water_Temperature_Hydroponics\:;
  payload += Water_Temperature_Hydroponics;


  payload += \Water_level_Microgreens\:;
  payload += Water_level_Microgreens;

  payload += \Water_level_Hydroponics\:;
  payload += Water_level_Hydroponics;

  payload += };

  char attributes[3000];
  long now = millis();

  if (now - lastData > 5000) {

    lastData = now;
    payload.toCharArray(attributes, 1000);
    client.publish(publishTopic, attributes);
    Serial.println(attributes);

  }

  


  //json message//

  //creamos el documento json
 DynamicJsonDocument doc(1024);
 // StaticJsonDocument<300> doc;





  // creamos los objectos json

   JsonObject rack1 = doc.createNestedObject(rack1);
   rack1[Ambient_humidity1]  = Ambient_humidity1;
   rack1[Ambient_humidity2] = Ambient_humidity2;
   rack1[Ambient_Temperature1] = Ambient_Temperature1;
   rack1[Ambient_Temperature2] = Ambient_Temperature2;
   rack1[Water_Temperature_Microgreens]  = Water_Temperature_Microgreens;
   rack1[Water_Temperature_Hydroponics] = Water_Temperature_Hydroponics;
   rack1[Water_level_Microgreens] = Water_level_Microgreens ;
   rack1[Water_level_Hydroponics] = Water_level_Hydroponics;
   rack1[soil_humidity_1a] = Soil_humidity_1a;
   rack1[soil_humidity_1b] = Soil_humidity_1b;
   rack1[soil_humidity_2a] = Soil_humidity_2a;
   rack1[soil_humidity_2b] = Soil_humidity_2b;


   //almacena el json en un buffer para enviar por mqttt

//   char buffer[256];
//   serializeJson(doc, buffer);
//   client.publish (esp32/satelitegrow, buffer);

   client.loop();



   delay (2000);


}



void waterpumpon_function () {

  digitalWrite(waterpump_hydro, LOW);
  Serial.println(hydroponic pump on);

}

void waterpumpoff_function () {
  digitalWrite(waterpump_hydro, HIGH);

  return;
}



void check_Soil_Humidity_Sensor_1a() {

  if  ( Soil_Humidity_Sensor_1a  >= 2200 &&  Soil_Humidity_Sensor_1a  <= 3300) {
    digitalWrite(waterpump_soil, LOW);
    digitalWrite(Solenoid_Valvule_1 , LOW);
    Serial.print(irrigation1 in process);

    delay(10000);
    digitalWrite(waterpump_soil, HIGH);
    digitalWrite(Solenoid_Valvule_1 , HIGH);
    Serial.println(rirrigation1 done);

  }

  else {
    Serial.println(humidity1 ok);

    return;
  }
}



void check_Soil_Humidity_Sensor_1b() {

  if  ( Soil_Humidity_Sensor_1b  >= 2200 && Soil_Humidity_Sensor_1b <= 3300) {

    digitalWrite(waterpump_soil, LOW);
    digitalWrite(Solenoid_Valvule_2 , LOW);
    Serial.print(irrigation2 in process);

    delay(10000);
    digitalWrite(waterpump_soil, HIGH);
    digitalWrite(Solenoid_Valvule_2 , HIGH);
    Serial.println(irrigation2 done);

  }
  else {
    Serial.println(humidity2 ok);

    return;
  }

}

void check_Soil_Humidity_Sensor_2a() {

  if ( Soil_Humidity_Sensor_2a  >= 2200 &&  Soil_Humidity_Sensor_2a  <= 3300) {
    digitalWrite(waterpump_soil, LOW);
    digitalWrite(Solenoid_Valvule_3 , LOW);
    Serial.print(irrigation3 in process);

    delay(10000);
    digitalWrite(waterpump_soil, HIGH);
    digitalWrite(Solenoid_Valvule_3, HIGH);
    Serial.println(irrigation3 done);

  }
  else {
    Serial.println(humidity3 ok);

    return;
  }
}



void check_Soil_Humidity_Sensor_2b()  {

  if ( Soil_Humidity_Sensor_2b  >= 2200 &&  Soil_Humidity_Sensor_2b  <= 3300) {
    digitalWrite(waterpump_soil, LOW);
    digitalWrite(Solenoid_Valvule_4 , LOW);
    Serial.print(irrigation4 in process);

    delay(10000);
    digitalWrite(waterpump_soil, HIGH);
    digitalWrite(Solenoid_Valvule_4 , HIGH);
    Serial.println(irrigation4 done);

  }

  else {
    Serial.println(humidity4 ok);

    return;
  }
}




void printTime()
{
  print2digits(rtc.gehours() + GMT);
  Serial.print(:);
  print2digits(rtc.getMinutes());
  Serial.print(:);
  print2digits(rtc.getSeconds());
  Serial.println();
}

void printDate()
{
  Serial.print(rtc.getDay());
  Serial.print(/);
  Serial.print(rtc.getMonth());
  Serial.print(/);
  Serial.print(rtc.getYear());

  Serial.print( );
}


void print2digits(int number) {
  if (number < 10) {
    Serial.print(0);
  }
  Serial.print(number);
}