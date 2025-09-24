#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "SparkFun_ENS160.h"
#include <Adafruit_AMG88xx.h>
#include <AHTxx.h>
#include <Wire.h>

const int sens_id = 1;
const char* sens_name = "Sensor Central";

const char* ssid = "iruMobil";
const char* password = "hexit564";

const char* serverName = "https://sparq-api-dpbdg9c9h3ehaub8.brazilsouth-01.azurewebsites.net/readings";

Adafruit_AMG88xx amg;
AHTxx aht20;
SparkFun_ENS160 ens160; 

void TCA9548A(uint8_t bus){
  Wire.beginTransmission(0x70); // Address for multiplexer
  Wire.write(1 << bus);
  Wire.endTransmission();
  //Serial.print(bus);
}

void setup() {
  Serial.begin(115200);

  Wire.begin();
  TCA9548A(2);
  if (!aht20.begin(0x38)) {
    Serial.println("Could not find a valid aht21 sensor on bus 2, check wiring!");
    while (1);
  }
  Serial.println("AHT20 initialized.");
  if (!ens160.begin()) {
    Serial.println("ENS160 not found. Check connections.");
    while (1);
  }
  Serial.println("ENS160 initialized.");

  if (ens160.setOperatingMode(0xF0))
    Serial.println("ENS soft reset.");

  delay(100);

  if (ens160.setOperatingMode(0x02))
      Serial.println("ENS in operation.");

  TCA9548A(3);

  if (!amg.begin()) {
      Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
      while (1);
  }

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 15000;


float ahtTemp;
float ahtHumi;
int ensco2;
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];

void loop() {
  //Sends an HTTP POST request for every timerDelay
  if ((millis() - lastTime) > timerDelay) {
    if(WiFi.status()== WL_CONNECTED){
      // WiFiClient client;
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
    
      http.begin(client, serverName);
      //http.setInsecure();

      TCA9548A(2);
      ahtTemp = aht20.readTemperature();
      if(ahtTemp == AHTXX_ERROR){
        Serial.print("Error on AHT temperature read. Attempting reset");
        if(aht20.softReset() == true) {
          Serial.println(F("reset success"));
        }else{
          Serial.println(F("reset failed"));
          while(1);
        }
      }

      delay(2000); // COOLING SOLUTION. DO NOT REMOVE!

      TCA9548A(2);
      ahtHumi = aht20.readHumidity();
      if(ahtHumi == AHTXX_ERROR){
        Serial.print("Error on AHT temperature read. Attempting reset");
        if(aht20.softReset() == true) {
          Serial.println(F("reset success"));
        }else{
          Serial.println(F("reset failed"));
          while(1);
        }
      }

      delay(2000); // COOLING SOLUTION. DO NOT REMOVE!

      if(ens160.checkDataStatus() )
      {
        // Serial.print("Air Quality Index (1-5) : ");
        // Serial.println(myENS.getAQI());

        // Serial.print("Total Volatile Organic Compounds: ");
        // Serial.print(myENS.getTVOC());
        // Serial.println("ppb");

        ensco2 = ens160.getECO2(); // ppm
      }

      delay(1500);

      TCA9548A(3);
      amg.readPixels(pixels);
      
    // Convert the pixels array to a JSON string
      String thermoMat = "[";
      for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
        thermoMat += String(pixels[i], 2); // 2 decimal places for floats
        if (i < AMG88xx_PIXEL_ARRAY_SIZE - 1) {
          thermoMat += ",";
        }
      }
      thermoMat += "]";

      // Build the payload with the matrix
      String payload = String("{\"sens_id\":") + sens_id +
            ",\"temp\":" + (ahtTemp * 100) +
            ",\"humi\":" + (ahtHumi * 100) +
            ",\"carb\":" + ensco2 +
            ",\"sens_name\":\"" + sens_name + "\"" +
            ",\"thermo_mat\":" + thermoMat +
            "}"; // matrix now included



      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(payload);

      delay(500);
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}