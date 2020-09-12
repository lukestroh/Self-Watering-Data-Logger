/* Automatic Garden Waterer and Data Logger

    code modified from: https://github.com/gradyh/GradyHillhouseGarduino/blob/master/GradyHillhouse_Garduino.ino   by Grady Hillhouse (2015)

    Luke Strohbehn (2020)

    Hardware:
      Arduino Uno
      Adafruit Data Logging Shield for Arduino PRODUCT ID: 1141
      Air Temperature/Humidity Sensor: DHT22
      Soil Temperature Sensor: DS18B20
      Soil Moisture Sensor: EK1940 (or some version of it)
      Photoresistor
      HC-06 Bluetooth Module



      ********************************* 
      * Things to improve:
      * Take 3 readings per cycle, average them
      * Get better sensors
      * Perhpas make it so that the Arduino creates a new file when it backs up to Python? Also, extending the number of files able to be created.
      *********************************
      
      
*/

#include "DHT.h"    // Air humidity/temperature logger, Adafruit library
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h" // Real Time Clock library (used for the datalogger)
#include "SD.h"     // SD card library (to talk to SDs)
#include "OneWire.h" // Library to talk to DS18B20 Soil Temperature Sensor (using an external
#include "DallasTemperature.h" // Library defining Dallas Sensors functions
#include "NeoSWSerial.h" // Library to communicate via Bluetooth with PC


#define DHTTYPE DHT22
#define ECHO_TO_SERIAL 0    /*   sends datalogging to serial if 1, nothing if 0. (This determines whether to send the stuff that's being written to the card to the Serial monitor. 
This makes the logger a little more sluggish, and you may want to use the serial monitor for other things. However, it is useful. Setting it to "0" will turn it off.  
#if and #endif allow comparison at compile time, rather than run time.
*/

// Time between logs
const unsigned long LOG_INTERVAL = 360000; // milliseconds between log entries (360000 ms = 6 min). Written as unsigned long to do arithmetic with millis().

// Declare pins
const int dht_pin = 2;
const int solenoid_pin = 3;
const int soil_temp_pin = 4;
const int LED_red_pin = 7;
const int chip_select_pin = 10;  // Data logger uses pin 10. Always.
const int RX_pin = 8;
const int TX_pin = 9;
const int soil_moisture_pin = A0;
const int sunlight_pin = A1;

// Define watering parameters
const int watering_threshold = 50;
const long watering_time = 1200000; // 20 minutes of watering

// Declare variables
float humidity = 0;                 // Relative air humidity
float air_temp = 0;                 // Relative air temperature
float heat_index = 0;               // Computed heat index
float soil_temp = 0;                // Soil temperature
float soil_moisture = 0;            // Normalized soil moisture data value
int dry_soil_value = 691;           // Maximum return value from analogRead(). Program can change this.
int wet_soil_value = 359;           // Minimum return value from analogRead(). Program can change this.
int soil_value_difference = dry_soil_value - wet_soil_value;
float sunlight = 0;                 // Normalized sunlight level
int sunlight_max = 881;             // Maximum return value from analogRead(). Program can change this.

bool watered_today;                 // Boolean for aknowledging if we've watered that day

String latest_data;                 // String for storing and printing our data in its entirety



// Variables for checking Unix time with Python
const byte numb_chars = 11;
char receivedChars[numb_chars];     // An array to store the received Unix time
bool newData = false;               // Boolean for aknowledging presence of data from Python
char char_unix[11];                 // Array for storing now.unixtime() as char



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     
          Instantiations
 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */




DHT dht(dht_pin, DHTTYPE);               // Setup a dht instance to communicate with DHT device
OneWire oneWire(soil_temp_pin);          // Setup a oneWire instance to communicate with any OneWire devices (for our soil temperature sensor)
DallasTemperature dtSensor(&oneWire);    // Pass our oneWire reference to our Dallas Temperature sensor
RTC_PCF8523 rtc;                         // Setup the Real Time Clock on our Adafruit Datalogger (our RTC is a PCF8523, as seen on the shield, be sure to check yours)
DateTime now;                            // Set clock to current time
File logfile;                            // Setup a logfile instance to communicate with our SD card
NeoSWSerial btSerial(RX_pin, TX_pin);    // Setup a btSerial instance to communicate with our Bluetooth chip





/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     
          Functions
 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


// Declare reset function at address 0
void (* resetFunc) (void) = 0; 



// See if the card is present and can be initialized.
void initializeSD() {
  if (!SD.begin(chip_select_pin)) {
    Serial.println(F("SD card failed, or not present"));
    btSerial.println(F("SD card failed, or not present"));
    // Flash red LED to indicate error, wait 5 sec, reboot
    int j = millis() + 5000;
    Serial.println(F("Insert card or reseat now. Reset pending..."));
    btSerial.println(F("Insert card or reseat now. Reset pending..."));
    do {
      digitalWrite(LED_red_pin, HIGH); delay(250); digitalWrite(LED_red_pin, LOW); delay(250);
    } while (j > millis());
    Serial.println(F("RESET"));
    btSerial.println(F("RESET"));
    delay(250);
    resetFunc(); // calls reset function to reboot Arduino
    
    return;
  }
  
  Serial.println(F("SD card initialized"));
}



// Create a new .csv file depending on if a file name is already present
void createFile() {
  // Create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {                 // This loop creates a file number from 'LOGGER00.CSV' to 'LOGGER99.CSV'
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (!SD.exists(filename)) {                       // Tests whether a file or directory exists on the SD card
      // Only open a file that doesn't already exist
      logfile = SD.open(filename, FILE_WRITE);        // Open the file for reading and writing, starting at the end of the file.
      break; // Leave the loop!
    }
  }


  if (!logfile) {
    Serial.println(F("Error: couldn't create file... Rebooting in 5 seconds"));
    delay(5000);
    resetFunc();
  }

  Serial.print(F("Logging to: "));
  Serial.println(filename);
}



// Get temperature, humidity, and heat index values from DHT sensor
void getDHT22Data() {
  humidity = dht.readHumidity();        // Get air humidity
  air_temp = dht.readTemperature();     // Get air temperature in Celsius (arg = 'true' if user wants Fahrenheit)
  heat_index = dht.computeHeatIndex(air_temp, humidity, false); // Get heat index ('false' if in Celsius)
  
  // Warn user that DHT22 sensor isn't working
  if (isnan(humidity) || isnan(air_temp) || isnan(heat_index)) {
//    Serial.println(F("Failed to read from DHT22 sensor!"));
    int i = 0;
    while (i < 3) {
      digitalWrite(LED_red_pin, HIGH);
      delay(100);
      digitalWrite(LED_red_pin, LOW);
      delay(100);
      i++;
    }
  }
}



// Get soil temperature data from DS18B20 sensor
void getDS18B20Data(){
  dtSensor.requestTemperatures();
  soil_temp = dtSensor.getTempCByIndex(0);


  // Warn user that DS18B20 sensor isn't working
  if (isnan(soil_temp) || soil_temp == -127.00) {
//    Serial.println(F("Failed to read DS18B20 sensor!"));
    int i = 0;
    while (i < 5) {
      digitalWrite(LED_red_pin, HIGH);
      delay(100);
      digitalWrite(LED_red_pin, LOW);
      delay(100);
      i++;
    }
  }
}



// Get soil moisture data from Capacitive Soil Moisture Sensor (Value_0: 580, Value_100: 305)
//***IMPORTANT***: These values can really change depending on how deep your soil sensor is buried, and your specific sensor. Be sure to calibrate for specific garden placement.
void getSoilMoistureData() {
  int soil_moisture_raw = analogRead(soil_moisture_pin);
  if (soil_moisture_raw > dry_soil_value) {
    dry_soil_value = soil_moisture_raw;
    soil_value_difference = dry_soil_value - wet_soil_value;
  }
  if (soil_moisture_raw < wet_soil_value) {
    wet_soil_value = soil_moisture_raw;
    soil_value_difference = dry_soil_value - wet_soil_value;
  }
  soil_moisture_raw = soil_moisture_raw - wet_soil_value;
  soil_moisture = (1-float(soil_moisture_raw)/soil_value_difference)*100;
}



// Get sunlight levels
// These values based on ADC resolution, which can be calculated as (ADC_resolution/System_voltage = ADC_reading/Analog_voltage_reading). This value was used for sunlight_max.
void getSunlightData() { 
  int sunlight_raw = 0;
  sunlight_raw = analogRead(sunlight_pin);
  if (sunlight_raw > sunlight_max) {
    sunlight_max = sunlight_raw;
  }
  sunlight = 100.0*sunlight_raw/sunlight_max;
}



// Water the garden
void waterGarden() {
    // Activate the solenoid valve, if watering deemed necessary
  if ((soil_moisture < watering_threshold) && (now.hour() > 19) && (now.hour() < 22) && (watered_today == false)) {
    // Turn on valve.
    digitalWrite(solenoid_pin, HIGH);
    digitalWrite(LED_red_pin, HIGH);
    Serial.println(F("Watering!"));
    delay(watering_time);
    digitalWrite(solenoid_pin, LOW);
    digitalWrite(LED_red_pin, LOW);
    watered_today = true;
  }
}



// Write out the header to the file (using CSV format)
void writeHeader() {
  logfile.println(F("Unix time (s),Date,Soil Temp (°C),Soil Moisture Content (%),Air Temp (°C),Relative Humidity (%),Heat Index (°C),Sunlight Illumination (%),Watered Today"));
//  btSerial.println("Unix time (s),Date,Soil Temp (°C),Soil Moisture Content (%),Air Temp (°C),Relative Humidity (%),Heat Index (°C),Sunlight Illumination (%),Watered Today");// 153 characters
  #if ECHO_TO_SERIAL
    Serial.println(F("Unix time (s),Date,Soil Temp (°C),Soil Moisture Content (%),Air Temp (°C),Relative Humidity (%),Heat Index (°C),Sunlight Illumination (%),Watered Today"));
  #endif
  if (logfile.getWriteError()) {
    Serial.println(F("Error: couldn't write header... Rebooting in 5 seconds"));
    delay(5000);
    resetFunc();
  }
}



// Log Date, variables, to the Serial monitor, to the SD card, and send via Bluetooth
void logLatestData() {

  // Reset char_unix array.
  memset(char_unix, '\0', sizeof(char_unix));
  long unix_time = now.unixtime();
  // Store unix_time into char_unix array. This value will be used to compare to the last stored data value from the Python code.
  ltoa(unix_time, char_unix,10);

  // Create our data String. If less intensive memory method can be used, this should be substituted.
  latest_data = unix_time;
  latest_data.concat(",");
  latest_data.concat(now.year());
  latest_data.concat("/");
  latest_data.concat(now.month());
  latest_data.concat("/");
  latest_data.concat(now.day());
  latest_data.concat(" ");
  latest_data.concat(now.hour());
  latest_data.concat(":");
  latest_data.concat(now.minute());
  latest_data.concat(":");
  latest_data.concat(now.second());
  latest_data.concat(",");
  latest_data.concat(soil_temp);
  latest_data.concat(",");
  latest_data.concat(soil_moisture);
  latest_data.concat(",");
  latest_data.concat(air_temp);
  latest_data.concat(",");
  latest_data.concat(humidity);
  latest_data.concat(",");
  latest_data.concat(heat_index);
  latest_data.concat(",");
  latest_data.concat(sunlight);
  latest_data.concat(",");
  latest_data.concat(watered_today);

  // Print data to the SD card, the Bluetooth serial port, and to the serial monitor, if ECHO_TO_SERIAL
  logfile.println(latest_data);
  delay(500);
  
  btSerial.println(latest_data);
  delay(500);
  
  #if ECHO_TO_SERIAL
    Serial.println(latest_data);
  #endif

} 



// Get data from Python using start marker '<' and end marker '>'
void recWithMarkers() {
  static boolean recv_in_prog = false;
  static byte ndx = 0;
  char start_marker = '<';
  char end_marker = '>';
  char btData;

  while (btSerial.available() > 0 && newData == false) {

    btData = btSerial.read();

    if (recv_in_prog) {
      if (btData != end_marker) {
        receivedChars[ndx] = btData;
        ndx++;
        if (ndx >= numb_chars) {
          ndx = numb_chars - 1;
        }
      } else {
        receivedChars[ndx] = '\0'; // terminate the string
        recv_in_prog = false;
        ndx = 0;
        newData = true;
      }
      
    }
    else if (btData == start_marker) {
      recv_in_prog = true;
    }
  }
}



// Check and see if Python script needs more than just recent data by comparing the unix time
void comparePythonData() {

  if (newData == true) {

    Serial.print(F("Char_unix: "));
    Serial.println(char_unix);
    Serial.print(F("receivedChars: "));
    Serial.println(receivedChars);

  
    if (strcmp(receivedChars, char_unix) != 0) {
//      Serial.println(F("Testing"));
      btSerial.println("False");
      btSerial.println(char_unix);
      logfile.seek(0);                    // Go to the beginning of the file
      while (logfile.available()) {       
        btSerial.write(logfile.read());   // print all lines to Bluetooth. The database will throw out unnecessary data lines.
      }
//      Serial.println();
//      Serial.println(F("End test"));

    }
    
    newData = false;
  }
}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     
          Setup and Loop
 
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void setup() {

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for Serial port to connect.
  }
  Serial.println();


  // Define pin modes
  pinMode(dht_pin, INPUT);
  pinMode(solenoid_pin, OUTPUT);
  pinMode(LED_red_pin, OUTPUT);
  pinMode(chip_select_pin, OUTPUT);
  pinMode(soil_moisture_pin, INPUT);  
  pinMode(sunlight_pin, INPUT);

  /* 
    Begin communications with the various shields and sensors attached to the Arduino
  */

  // Establish connection with DHT sensor
  dht.begin();
  
  // Establish connection with Dallas Temperature sensor            
  dtSensor.begin();

  // Establish connection with RTC, send message if unsuccessful
  Wire.begin();           
  if (!rtc.begin()) {
    logfile.println(F("RTC failed"));
  #if ECHO_TO_SERIAL
    Serial.println(F("RTC failed"));
  #endif
  }

  delay(1000);
  
  // begin bluetooth Serial connection
  btSerial.begin(9600);

  delay(3000);

  // Set the time and date on the RTC, if necessary
  if ( !rtc.initialized() || rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // This line sets the RTC to the Date and Time that the sketch was compiled.
  }


  // Get logger ready to log
  initializeSD();
  createFile();
  writeHeader();

}



void loop() {

  // Delay software
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  

  // Check if Python data is most recent
  recWithMarkers();
  comparePythonData();
  

  // Three blinks means the start of a log.
  digitalWrite(LED_red_pin, HIGH);
  delay(1000);
  digitalWrite(LED_red_pin, LOW);
  delay(1000);
  digitalWrite(LED_red_pin, HIGH);
  delay(1000);
  digitalWrite(LED_red_pin, LOW);
  delay(1000);
  digitalWrite(LED_red_pin, HIGH);
  delay(1000);
  digitalWrite(LED_red_pin, LOW);


  // Reset watered_today if new day
  if (!(now.day() == rtc.now().day())) {  
    watered_today = false;
  }

  
  // Water the garden, if needed
  waterGarden();


  // Update the time
  now = rtc.now();

  
  // Get sensor data
  getDHT22Data();
  getDS18B20Data();
  getSoilMoistureData();
  getSunlightData();


  // Log the data
  logLatestData();
  
  // Write to SD card
  logfile.flush();
  
//  if (logfile.getWriteError()) {
//    Serial.println(F("Error: couldn't log data... Rebooting in 5 seconds"));
//    delay(5000);
//    resetFunc();
//}

}
