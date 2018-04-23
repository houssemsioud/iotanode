#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#define DHTPIN            2         // Pin which is connected to the DHT sensor.

#define DHTTYPE           DHT11     // DHT 11 temperature and humidity sensor
DHT_Unified dht(DHTPIN, DHTTYPE);

const char* host = "esp8266-webupdate"; //host name
const char* ssid = "Lenovo";			//wifi ssid
const char* password = "88888888";		//wifi password
const char* mqtt_server = "broker.mqtt-dashboard.com";



unsigned int localPort = 2390;					       	//		-------
IPAddress timeServerIP;							//		-------	
const char* ntpServerName = "time.nist.gov";				//		-------
const int NTP_PACKET_SIZE = 48;					      	//	       NTP packet 
unsigned long previousMillis=0;					      	//		-------
byte packetBuffer[ NTP_PACKET_SIZE];			    		//		-------
WiFiUDP udp;								//		-------



ESP8266WebServer httpServer(80);				//				HTTP update
ESP8266HTTPUpdateServer httpUpdater;			    	//				initializing

WiFiClient espClient;							 //				WIFI	
PubSubClient client(espClient);				      		// 				init

long lastMsg = 0;
char msg[200];
char msg1[200];
char temp[5];
char hum[5];
char time_str[100];
int value = 0;
unsigned long zeroMillis;
bool a = false;
unsigned long epoch;
unsigned long timer;
int switchPin = 14;
uint32_t delayMS;
//----------------------------- Message arrived from subscribed topic --------------------------
void callback(char* topic, byte* payload, unsigned int length) {											
  Serial.print("Message arrived [");																		
  Serial.print(topic);																						
  Serial.print("] ");																						
  for (int i = 0; i < length; i++) {																		
    Serial.print((char)payload[i]);																			
  }																											
  Serial.println();																							
																											
  // Switch on the LED if an 1 was received as first character												
  if ((char)payload[0] == '1') {																			
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level				
    // but actually the LED is on; this is because															
    // it is acive low on the ESP-01)																		
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}
//----------------------------- Connecting to MQTT broker ------------------------------------
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-demo";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("00789/init", "hello world");
      // ... and resubscribe
      client.subscribe("00779/msg_in");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//------------------------------- Sending NTP packet to receive time from server ---------------------------------
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.print(".");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:

  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
//--------------------------------- Converting epoch to UTC iso format ------------------------------------
const char * datetime (){
  long now = millis();
  int a= (now- zeroMillis) / 1000  ;
  timer = epoch + a;
  snprintf (time_str, 100, "%02d-%02d-%02dT%02d:%02d:%02d", year(timer), month(timer),day(timer),hour(timer),minute(timer),second(timer));
  return time_str;
}
//--------------------------------- Getting light intensity values from sensor---------------------------
int light(){
  	int sensorValue = analogRead(A0);
    return sensorValue;
}
//--------------------------------- Switching state of occupancy --------------------------
void occupancy()
    {
     a = !a ;

    }
//---------------------------------- Setup-----------------------------------------------------
void setup(void) {
  pinMode(switchPin, INPUT_PULLUP);  // occupancy sensor pin

  dht.begin();
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);

  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);			
  WiFi.begin(ssid, password);			// begin wifi connection

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {			// reconnect if wifi not available
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  MDNS.begin(host);							//begin DNS for http update

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host); // printing URL for HTTP update

  udp.begin(localPort);					//begin udp for NTP packet
  
  client.setServer(mqtt_server, 1883);			// MQTT server connection
  client.setCallback(callback);
  
  //---------------------------------- printing sensor setup defaults -------------------------
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
  //----------------------------------------------------------------------------------------------
  //-------------------- NTP server setup and getting epoch --------------------------------------------
  WiFi.hostByName(ntpServerName, timeServerIP); 			
  Serial.print("Getting time from server");
  while(1){
  sendNTPpacket(timeServerIP);
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.print(".");
  }
  else {
    zeroMillis = millis();
    udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.println("");
    Serial.print("Unix time = ");
    const unsigned long seventyYears = 2208988800UL;
    epoch = secsSince1900 - seventyYears;
    Serial.println(epoch);
    break;
}
}
}

void loop(void) {
	
  httpServer.handleClient();   //retry MQTT server connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();

  delay(delayMS);
  // --------------------------- Get temperature event and print its value.----------------------------
  sensors_event_t event;

  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    Serial.print("Temperature: ");
    Serial.print(event.temperature);
    snprintf (temp, 5, "%ld",  (int)event.temperature);
    Serial.println(" *C");
  }
  // ------------------------------ Get humidity event and print its value.-------------------------------
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    Serial.print("Humidity: ");
    Serial.print(event.relative_humidity);
    snprintf (hum, 5, "%ld", (int)event.relative_humidity);
    Serial.println("%");
  }
  if (digitalRead(switchPin)){occupancy();};
  
//----------------------------------- Publishing Json messages containing sensor values , time and topic -------------------------------------------
  if (now - lastMsg > 2000) {			// setting delay between sending MQTT messages
    lastMsg = now;
    ++value;
    String payload="{\"measurement\": \"temperature\",\"time\":\""+String(datetime())+"\",\"tags\":{\"station\":\"nodemcu\"},\"fields\":{\"value\":"+String(temp)+"}} ";
    if (client.publish("0079/temp", payload.c_str()) == true) {
      Serial.println("Success sending message temperature");
    }
    payload="{\"measurement\": \"Humidity\",\"time\":\""+String(datetime())+"\",\"tags\":{\"station\":\"nodemcu\"},\"fields\":{\"value\":"+String(hum)+"}} ";
    if (client.publish("0079/hum", payload.c_str()) == true) {
      Serial.println("Success sending message humidity");
    }
    payload="{\"measurement\": \"occupancy\",\"time\":\""+String(datetime())+"\",\"tags\":{\"station\":\"nodemcu\"},\"fields\":{\"value\":"+String(a)+"}} ";
    if (client.publish("0079/occ", payload.c_str()) == true) {
        Serial.println("Success sending message occupancy");
    }
    payload="{\"measurement\": \"brightness\",\"time\":\""+String(datetime())+"\",\"tags\":{\"station\":\"nodemcu\"},\"fields\":{\"value\":"+String(light())+"}} ";
    if (client.publish("0079/light", payload.c_str()) == true) {
        Serial.println("Success sending message light");
    }
}


}
