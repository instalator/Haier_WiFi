#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "........";
const char* password = "........";
const char* mqtt_server = "192.168.1.190"; //Сервер MQTT

IPAddress ip(192,168,1,242); //static IP
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiClient espClient;
PubSubClient client(espClient);
long prev = 0;
char msg[50];

#define id_connect "myhome-Conditioner"

int incomingByte = 0;
//FF FF 0A 00 00 00 00 00 01 01 4D 01 5A
byte qstn[] = {10,0,0,0,0,0,1,1,77,1}; //Команда запроса
byte start[] = {255,255};
byte data[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,,0,0,0,0,0,0,0,0,0,0,0}; //Массив данных

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);

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
    if (client.connect(id_connect)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("myhome/Conditioner/connection", "true");

      // ... and resubscribe
      client.subscribe("myhome/Conditioner/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

int Length(arr){
  return sizeof(arr)/sizeof(byte)
}

byte getCRC(byte req[]){
  byte crc = 0;
  for (int i=0; i<Length(req[]); i++){
    if (req[i] !== 0){
      crc += req[i];
    }
  }
  return crc
}

void SendData(byte req[]){
  Serial.write(start, 2);
  Serial.write(req, Length(req));
  Serial.write(getCRC(req));
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  Serial.print(topic);
  Serial.print("=");
    String strTopic = String(topic);
    String strPayload = String((char*)payload);
  Serial.println(strPayload);
  //callback_iobroker(strTopic, strPayload);

}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if(Serial.available() >= 33){
    for(byte i = 0; i < 33; i++){
      data[i] = Serial.read();
    }
    while(Serial.available()){
      delay(2);
      Serial.read();
    }
    Serial.write(data, Length(data));
  }
  if (!client.connected()){
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - prev > 2000) {
    prev = now;
    SendData(qstn); //Опрос кондиционера
  }
}
