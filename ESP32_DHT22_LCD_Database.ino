/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com
*********/

#include <WiFi.h>
#include "DHT.h"
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

IPAddress server_addr(10,19,200,60);  // IP of the MySQL *server* here
char user[] = "namiki";              // MySQL user login username
char password[] = "namiki";        // MySQL user login password

// Sample query
char INSERT_SQL[] = "INSERT INTO `dbtest`.`device01` (`Datetime`, `Temperature`, `Humidity`) VALUES (NOW(), %s, %s)";
char query[256];

WiFiClient client; 
MySQL_Connection conn(&client);
MySQL_Cursor* cursor;

// DHT Sensor
#define DHTPIN 4     // what digital pin we're connected to

// Uncomment one of the lines below for whatever DHT sensor type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Replace with your network details
const char* ssid = "NPT-CEO_2.4Ghz";
const char* pass = "2021020100";

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

// only runs once on boot
void setup() {  
  // Initializing serial port for debugging purposes
  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect. Needed for Leonardo only

  dht.begin();
  
  lcd.begin();
  lcd.backlight();
  
  // Begin WiFi section
  Serial.printf("\nConnecting to %s", ssid);
  WiFi.begin(ssid, pass);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    i++;
    if (i == 20) {
      ESP.restart();
    }
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected");

  // Starting the web server
  Serial.println("...running. Waiting for the ESP IP...");
  delay(10000);

  // Printing the ESP IP address
  Serial.println(WiFi.localIP());
  lcd.setCursor(2, 1);
  lcd.print(WiFi.localIP());

  Serial.print("Connecting to SQL...  ");
  if (conn.connect(server_addr, 3306, user, password))
    Serial.println("OK.");
  else
    Serial.println("FAILED.");
  
  // create MySQL cursor object
  cursor = new MySQL_Cursor(&conn);
  
  delay(3000);
  lcd.clear();
  
  xTaskCreatePinnedToCore(RecorddatabaseTask, "recorddatabase_task", 10000, NULL, 5, NULL, 0);
  xTaskCreatePinnedToCore(dhtTask, "dhtTask", 10000, NULL, 5, NULL, 1);
}

void loop() {}

void dhtTask(void *pvParameter) {
    while(true)  {
        // Wait a few seconds between measurements.
        delay(2000);
      
        // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        float h = dht.readHumidity();
        // Read temperature as Celsius (the default)
        float t = dht.readTemperature();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        float f = dht.readTemperature(true);
      
        // Check if any reads failed and exit early (to try again).
        if (isnan(h) || isnan(t) || isnan(f)) {
          Serial.println(F("Failed to read from DHT sensor!"));
          strcpy(celsiusTemp, "Failed");
          strcpy(fahrenheitTemp, "Failed");
          strcpy(humidityTemp, "Failed");
        }
      
        // Compute heat index in Fahrenheit (the default)
        float hif = dht.computeHeatIndex(f, h);
        // Compute heat index in Celsius (isFahreheit = false)
        float hic = dht.computeHeatIndex(t, h, false);
      
        Serial.print(F("Humidity: "));
        Serial.print(h);
        Serial.print(F("%  Temperature: "));
        Serial.print(t);
        Serial.print(F("°C "));
        Serial.print(f);
        Serial.print(F("°F  Heat index: "));
        Serial.print(hic);
        Serial.print(F("°C "));
        Serial.print(hif);
        Serial.println(F("°F"));
        
        // Computes temperature values in Celsius + Fahrenheit and Humidity
        dtostrf(t, 6, 2, celsiusTemp);
        dtostrf(f, 6, 2, fahrenheitTemp);
        dtostrf(h, 6, 2, humidityTemp);
        
        lcd.setCursor(0, 0);
        lcd.print("Temp   :       ");
        lcd.setCursor(9, 0);
        lcd.print(t);
        lcd.setCursor(14, 0);  
        lcd.print("C");
        lcd.setCursor(0, 1);
        lcd.print("Humid  :       ");
        lcd.setCursor(9, 1);
        lcd.print(h);
        lcd.setCursor(14, 1);
        lcd.print("%");
    }
}

void RecorddatabaseTask(void *pvParameter) {
    while(true)  {
        delay(10000);
        if (conn.connected()) {
            if (strcmp(celsiusTemp, "nan") != 0) {
                // Initiate the query class instance
                MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
                // Save
                sprintf(query, INSERT_SQL, celsiusTemp, humidityTemp);
                // Execute the query
                cur_mem->execute(query);
                // Note: since there are no results, we do not need to read any data
                // Deleting the cursor also frees up memory used
                delete cur_mem;    
                Serial.println(query);
                Serial.println("Data recorded.");
            }            
        } else {
            Serial.println("Not Connect Database...");
            ESP.restart();
        }
    }
}

