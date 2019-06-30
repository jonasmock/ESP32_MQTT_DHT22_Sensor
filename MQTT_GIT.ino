//////////Includes/////////////

// WiFi connection
#include <WiFi.h>
// MQTT CLient
#include <PubSubClient.h>
// Temp / Humi Sensor DHT22
#include <DHT.h>
// JSON to send data via MQTT
#include <ArduinoJson.h>
// NTP client for current time / timestamp
#include <NTPClient.h>


//////////Variables/////////////

// WiFi credentials
const char* ssid     = "SSID";
const char* password = "PASSWORD";

// MQTT Client broker and credentials
const char* mqttBroker = "XXX.XXX.XXX.XXX";
const int mqttPort = 1883;
const char* mqttUser = "yourMQTTuser";
const char* mqttPassword = "yourMQTTpassword";
const char* sensor = "SENSOR";
const char* mqttPublishTopic = "test/topic";

// Example MQTT Json message
const char* exampleMQTT = "{\"sensor\":\"esp32_01\",\"time\":1561925850,\"data\":[58,29.4]}";
// Calculate needed JSON document bytes with example message
const size_t CAPACITY = JSON_OBJECT_SIZE(sizeof(exampleMQTT) + 20 ); 

// DHT22 sensor type and which gpio pin is used for sensor data
#define DHTPIN 27 
#define DHTTYPE DHT22


//////////Objects/////////////

// Creates WiFi instance and passes it on as a parameter for the MQTT client object
WiFiClient espClient;
PubSubClient client(espClient);

// Creats sensor instance
DHT dht(DHTPIN, DHTTYPE);

// Creates WiFi UDP instance and passes it on as a parameter for the NTP client object
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);


//////////Functions/////////////

// Function for setting up a WiFi connection
void setupWiFi()
{
    // Serial information
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    // Initiates connection
    WiFi.begin(ssid, password);

    // As long as WiFi status is not connected a "loading bar" is displayed in serial terminal
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // WiFi status information
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());  
}

// Function for setting up MQTT client
void setupMqtt()
{
    // Defines MQTT broker and port
    client.setServer(mqttBroker, mqttPort);
 
    // As long as MQTT client is not connected a "loading bar" is displayed in serial terminal
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        delay(500);
        Serial.print(".");

        // Initiates connection to MQTT broker. Unique id, user and password.
        if (client.connect("ESP32Client", mqttUser, mqttPassword )) {

            // If client is connected some status information are displayed
            Serial.println("...Connected to MQTT");
            Serial.println("Server: ");
            Serial.println(mqttBroker);
            Serial.println("Port: ");
            Serial.println(mqttPort); 

        // If not display client state and try again
        } else {
 
        Serial.print("Failed with state");
        Serial.print(client.state());
        delay(2000);
 
        }
    }
}

// Runs one time at startup
void setup()
{
    // Initializes serial terminal
    Serial.begin(115200);
    delay(10);
    // Initializes DHT22 sensor
    dht.begin();
    // Calls function to set up WiFi connection
    setupWiFi();
    // Initializes NTP client
    timeClient.begin();
    // Calls function to set up MQTT connection
    setupMqtt();  
}

// Runs in a loop
void loop()
{
    // Checks wether WiFi connection is established, else it calls function to set up WiFi connection
    if (WiFi.status() != WL_CONNECTED){
        Serial.println("\nTrying to connect to WiFi");
        setupWiFi();
    }

    // As long as NTP client is not up to date it should be forced to get current time
    while(!timeClient.update()) {
        timeClient.forceUpdate();
        delay(500);
        Serial.print(".");
    }

    // Once MQTT client is connected, is has to be refreshed every loop
    client.loop();

    // Requests humidity data from DHT22 sensor
    float h = dht.readHumidity();
    // Requests temperature data in Celsius from DHT22 sensor
    float t = dht.readTemperature();
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Creates JSON document with capacity from example message. Contains sensor name and timestamp. 
    StaticJsonDocument<CAPACITY> doc;
    doc["sensor"] = sensor;
    doc["time"] = timeClient.getEpochTime();
    // Add sensor data
    JsonArray data = doc.createNestedArray("data");
    data.add(h);
    data.add(t);
    // Serialize JSON doc to char buffer with variable capacity (MQTT client needs char / char*)
    char JSONmessageBuffer[CAPACITY];
    //serializeJson(doc, Serial);
    serializeJson(doc, JSONmessageBuffer);
    // Publishes JSON to defined MQTT topic
    client.publish(mqttPublishTopic, JSONmessageBuffer);
    doc = NULL;

    // Defines the repetition rate in milliseconds
    delay(5000);
}
