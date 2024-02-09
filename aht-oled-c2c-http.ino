#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>

// WiFi credentials
const char* ssid = "pewpew";
const char* password = "PewPewPew";

Adafruit_AHTX0 aht;
// OLED Display Settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readAHTTemperature() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  if (isnan(temp.temperature)) {
    Serial.println("Failed to read temperature from AHT sensor!");
    return "--";
  } else {
    float tempF = temp.temperature * 9.0 / 5.0 + 32;  // Convert to Fahrenheit
    return String(tempF);
  }
}

String readAHTHumidity() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  if (isnan(humidity.relative_humidity)) {
    Serial.println("Failed to read humidity from AHT sensor!");
    return "--";
  } else {
    return String(humidity.relative_humidity);
  }
}

void updateDisplay(String temperature, String humidity) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Len's Office");
  display.setTextSize(2);
  display.print("T:");
  display.print(temperature);
  display.println("F");
  display.print("H:");
  display.print(humidity);
  display.println("%");
  display.display();
}

String processor(const String& var) {
  if (var == "TEMPERATURE") {
    return readAHTTemperature();
  } else if (var == "HUMIDITY") {
    return readAHTHumidity();
  }
  return String();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" 
        integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" 
        crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    body {
      background-color: darkgrey; /* Change background color here */
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels {
      font-size: 1.5rem;
      vertical-align: middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Len's Office Temperature</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;F</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
</body>
<script>
setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000);

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000);
</script>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);

  // Explicitly define I2C pins
  Wire.begin(19, 18); // SDA, SCL

  // Initialize OLED display first
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Infinite loop on failure
  }
  display.display();
  delay(2000); // Show initial screen

  // Then initialize AHT10 sensor
  if (!aht.begin()) {
    Serial.println(F("Failed to initialize AHT10!"));
    // Handle AHT10 initialization failure here
    // For example, you could continue without the sensor, but you might want to display an error message on the OLED.
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(WiFi.localIP());

  // Define server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", readAHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", readAHTHumidity().c_str());
  });

  server.begin(); // Start server
}

void loop() {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();

  // Update sensor readings and display every 10 seconds
  if (currentMillis - lastUpdate >= 10000) { // 10 seconds interval
    lastUpdate = currentMillis;

    // Read sensor values
    String temperature = readAHTTemperature();
    String humidity = readAHTHumidity();

    // Update display
    updateDisplay(temperature, humidity);
  }
}
