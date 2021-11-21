/************************
 * Codigo para ser usado con el Home Assistant y el NodeMCU para los jardines.
 *  MIde el caudal y el consumo de agua.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
const char* ssid = "COLOCAR EL SSID DE LA RED INALAMBRICA";
const char* password = "COLOCAR LA CONTRASEÑA DE LA RED INALAMBRICA";
const char* mqtt_server = "COLOCAR LA IP DEL LA RASPBERRY PI";

WiFiClient espClient;
PubSubClient client(espClient);



///Suelos///
const int SueloPin1=A0;
const int SueloPin2=D0;



// valor de ajuste del umbral, para la sensibilidad del sensor
int thresholdValue = 800;


//Humedad del suelo del jardin
const char* topicNodeMCU1SueloM1 = "Home/NodeMCU1/Suelo1";
/////caudales
const char* topicCaudalP = "Home/Jardin2/CaudalP";
const char* topicConsumoP = "Home/Jardin2/ConsumoP";

/////caudal
void ISRCountPulse2()
{
   pulseConter2++;
}
 
float GetFrequency2()
{
   pulseConter2 = 0;
 
   interrupts();
   delay(measureInterval);
   noInterrupts();
 
   return (float)pulseConter2 * 1000 / measureInterval;
}
 
void SumVolume2(float dV)
{
   volume2 += dV / 60 * (millis() - t02) / 1000.0;
   t02 = millis();
}
////////////////////////

void setup_wifi() {
 Serial.begin(115200);
  delay(100);
 
  // We start by connecting to a WiFi network
 
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.subscribe("ha/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


////////////////////////

void setup()
{

  /////caudales
  attachInterrupt(digitalPinToInterrupt(caudalPinP), ISRCountPulse2, RISING);
  t02 = millis();

  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
  pinMode(SueloPin1, INPUT); 
  Serial.begin(9600);
}

void loop()
{
  if (!client.connected()) {
    reconnect();

  }
  else 
  {
   char buffer[40];

///////////Caudal
   float frequency2 = GetFrequency2();
 
   // calcular caudal L/min
   float flow_Lmin2 = frequency2 / factorK;
   SumVolume2(flow_Lmin2);
 
   Serial.print(" Caudal: P ");
   Serial.print(flow_Lmin2, 3);
   Serial.print(" (L/min)\tConsumo P:");
   Serial.print(volume2, 1);
   Serial.println(" (L)");
  

   // char buffer[35];
   dtostrf(flow_Lmin2,0, 1, buffer);
   client.publish(topicCaudalP, buffer);

   dtostrf(volume2,0, 1, buffer);
   client.publish(topicConsumoP, buffer);

///////////////////
 
    bool flag;
    
    /////////////Humedad 1/////////////
    int sensorValue1 = analogRead(SueloPin1);
    Serial.print(sensorValue1);
    if(sensorValue1 < thresholdValue)
    {
     
        
         Serial.println(" – Suelo húmedo, no necesita regarse");
         int valorHumedad1 = map(sensorValue1, 500, 1000, 100, 0);
         dtostrf(valorHumedad1,0, 0, buffer);
         client.publish(topicNodeMCU1SueloM1, buffer);


          delay(1000);
    }
    else
    {

          Serial.println(" – Suelo seco, necesita regarse");
          int valorHumedad1 = map(sensorValue1, 5000, 1023, 100, 0);
          dtostrf(valorHumedad1,0, 0, buffer);
          client.publish(topicNodeMCU1SueloM1, buffer);


          delay(1000);
      
    }
   
  // Tiempo entre envios (en ms)
  delay(5000);
 }
}
