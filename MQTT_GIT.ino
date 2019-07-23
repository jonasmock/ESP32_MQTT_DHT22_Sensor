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
const char* password = "SuperSafePassword";

// MQTT Client broker and credentials
const char* mqttBroker = "x.x.x.x";
const int mqttPort = 1883;
const char* mqttUser = "yourMQTTuser";
const char* mqttPassword = "yourMQTTpassword";
const char* sensor = "SENSOR_NAME";
const char* mqttPublishTopic = "SENSOR_NAME/sensor";

// Example MQTT Json message
const char* exampleMQTT = "{\"sensor\":\"esp32_01\",\"time\":1561925850,\"data\":[58.3,29.4,3.3]}";
// Calculate needed JSON document bytes with example message
const size_t CAPACITY = JSON_OBJECT_SIZE(sizeof(exampleMQTT) + 20); 

// DHT22 sensor type and which gpio pin is used for sensor data
#define DHTPIN 27 
#define DHTTYPE DHT22

// DEEP Sleep settings
#define TIME_TO_SLEEP  3600000000        /* Time ESP32 will go to sleep (in micro seconds) */

//////////Objects/////////////

// Creates WiFi instance and passes it on as a parameter for the MQTT client object
WiFiClient espClient;
PubSubClient client(espClient);

// Creats sensor instance
DHT dht(DHTPIN, DHTTYPE);

// Creates WiFi UDP instance and passes it on as a parameter for the NTP client object
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp2.fau.de", 7200);


//////////Functions/////////////

// Function for setting up a WiFi connection
void setupWiFi()
{
    // Serial information
    Serial.println();
    Serial.println();
    Serial.print("STATUS: Connecting to ");
    Serial.println(ssid);

    // Initiates connection
    WiFi.begin(ssid, password);

    // As long as WiFi status is not connected a "loading bar" is displayed in serial terminal
    while (WiFi.status() != WL_CONNECTED) {
        delay(10000);
        Serial.print("#");
        WiFi.begin(ssid, password);
    }

    // WiFi status information
    Serial.println();
    Serial.println("STATUS: WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();  
}

// Function for setting up MQTT client
void setupMqtt()
{
    // Defines MQTT broker and port
    client.setServer(mqttBroker, mqttPort);
    Serial.println("STATUS: Connecting to MQTT");
 
    // As long as MQTT client is not connected a "loading bar" is displayed in serial terminal
    while (!client.connected()) {
        delay(10000);
        Serial.print("#");
        client.connect("ESP32Client", mqttUser, mqttPassword);
    }

    // If client is connected some status information are displayed
    Serial.println();
    Serial.println("STATUS: Connected to MQTT");
    Serial.print("Server: ");
    Serial.println(mqttBroker);
    Serial.print("Port: ");
    Serial.println(mqttPort);
    Serial.println(); 
}

// Runs one time at startup
void setup()
{
    // Initializes serial terminal
    Serial.begin(115200);
    delay(10);

    // Enable sleep timer
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP);

    // Sleep config: disables additional components which aren't necessary to safe energy
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    Serial.println("Configured all RTC Peripherals to be powered down in sleep");
    
    // Initializes DHT22 sensor
    dht.begin();
    
    // Calls function to set up WiFi connection
    setupWiFi();
    
    // Initializes NTP client
    timeClient.begin();
    
    // Calls function to set up MQTT connection
    setupMqtt();  

    // Check if time is up to date
    if (timeClient.update()){
        Serial.println("STATUS: Time up to date");
    }else{
        //Update time
        while(!timeClient.update()) {
        Serial.print("#NTP#");
        delay(10000);
        }
    }

    // Requests humidity data from DHT22 sensor
    float h = dht.readHumidity();
    // Requests temperature data in Celsius from DHT22 sensor
    float t = dht.readTemperature();
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
        Serial.println("STATUS: Failed to read from DHT sensor!");
        float h = 0.0;
        float t = 0.0;
    }

    // Battery is connected to PIN35. Reads voltage and convert it to a readable value. (Is able to read between 0 and 3.3V. 0 = 0 / 3.3 = 4095)
    float voltage = analogRead(35)*(3.3/4095);
    if (voltage > 0.0){
      voltage = (float)((int)(voltage*100))/100;
    }

    // Creates JSON document with capacity from example message. Contains sensor name, timestamp and data. 
    StaticJsonDocument<CAPACITY> doc;
    doc["sensor"] = sensor;
    doc["time"] = timeClient.getEpochTime();
    doc["hum"] = h;
    doc["temp"] = t;
    doc["volt"] = voltage;
    
    // Serialize JSON doc to char buffer with variable capacity (MQTT client needs char / char*)
    char JSONmessageBuffer[CAPACITY];
    //serializeJson(doc, Serial);
    serializeJson(doc, JSONmessageBuffer);
    
    // Publishes JSON to defined MQTT topic
    while(!client.publish(mqttPublishTopic, JSONmessageBuffer)) {
        delay(10000);
        Serial.print("#MQTT#");
        if (!client.connected()){
          setupMqtt(); 
        }
    }
    Serial.println("STATUS: Sent data via MQTT");
    doc = NULL;

    Serial.println("Going to sleep now");
    delay(1000);
    Serial.flush();
    // Starts deep sleep. Hope we'll se us again.
    esp_deep_sleep_start();
    Serial.println("This will never be printed");

}

// Runs in a loop / Will not be reached in deep sleep scenario 
void loop()
{
    
}
