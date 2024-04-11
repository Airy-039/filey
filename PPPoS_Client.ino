#include <PPPoS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

#define PPP_APN "hubblethings.io"
#define PPP_USER ""
#define PPP_PASS ""
const char* otaServer = "petpooja-app-builds.s3.ap-south-1.amazonaws.com";
const char* otaFilePath = "/payroll/Payroll_Firmware_v1.0.ino.bin";

PPPoS ppp;
#define PWRKEY 2  // pg@30 hardware manual
#define RST_PIL 12
#define ESP_GPIO_RX 32
#define ESP_GPIO_TX 33
void Ini_() {
  pinMode(PWRKEY, OUTPUT);
  digitalWrite(PWRKEY, LOW);
  delay(550);
  digitalWrite(PWRKEY, HIGH);

  pinMode(RST_PIL, OUTPUT);
  digitalWrite(RST_PIL, LOW);
  delay(150);
  digitalWrite(RST_PIL, HIGH);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, ESP_GPIO_RX, ESP_GPIO_TX);

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(500);
  digitalWrite(2, HIGH);

  // Send AT command via Serial to Cavli module
  Serial.println("Sending ATI command to Cavli module");
  Serial.println("==================================");
  Serial1.println("ATI");  // Request manufacturer specific information about the TA.(terminal adaptre)
  delay(1000);
  Serial1.println("AT+PPPSTART");  // Request manufacturer specific information about the TA.(terminal adaptre)
  delay(1000);
}

void performOTAUpdate() {
  Serial.println("Starting OTA update...");

  // Construct the full URL for the firmware binary
  String otaURL = String(otaServer) + otaFilePath;

  // Begin HTTP client
  HTTPClient http;
  http.begin(otaURL);

  // Start the update process
  if (Update.begin(UPDATE_SIZE_UNKNOWN)) {
    Serial.println("Downloading...");

    // Start the download
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      WiFiClient& stream = http.getStream();
      uint8_t buffer[1024];
      int bytesRead;

      // Write the stream to the Update library in chunks
      while ((bytesRead = stream.readBytes(buffer, sizeof(buffer))) > 0) {
        if (Update.write(buffer, bytesRead) != bytesRead) {
          Serial.println("Error during OTA update. Please try again.");
          Update.end(false); // false parameter indicates a failed update
          return;
        }
      }

      // End the update process
      if (Update.end(true)) {
        Serial.println("OTA update complete. Rebooting...");
        ESP.restart();
      } else {
        Serial.println("Error during OTA update. Please try again.");
        Update.end(false); // false parameter indicates a failed update
      }
    } else {
      Serial.println("Failed to download firmware.");
      Update.end(false); // false parameter indicates a failed update
    }
  } else {
    Serial.println("Failed to start OTA update.");
  }

  // End HTTP client
  http.end();
}
void setup() {

  Ini_();
  delay(1000);
  Serial1.setTimeout(10);
  Serial1.setRxBufferSize(2048);
  ppp.begin(&Serial1);

  Serial.print("Connecting PPPoS");
  ppp.connect(PPP_APN, PPP_USER, PPP_PASS);
  while (!ppp.status()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("OK");
  delay(1000);

  // Check plain TCP connection
  requestJSON("http://httpbin.org/anything?client=ESP32_PPPoS");

  // Check secure SSL/TLS connection
  requestJSON("https://www.howsmyssl.com/a/check");
  test_("http://example.org");
  requestJSON("https://raw.githubusercontent.com/rajmehta28599/OTA_test_ESP32/main/sketch_apr3a.ino.bin");
  test_("https://drive.google.com/uc?export=download&id=13y5PMM3qBeANLXPanVUgkU_5iSD_NEyo");
  requestJSON("https://petpooja-app-builds.s3.ap-south-1.amazonaws.com/payroll/Payroll_Firmware_v1.0.ino.bin");
  // https://drive.google.com/uc?export=download&id=1-u2FYPCXY5-IvacgLHGuLqQUYnUL6y25
  Serial.println("Waiting 5 seconds before starting OTA update...");
  delay(5000);  // Wait for 5 seconds

  performOTAUpdate();


  ppp.end();
}

void loop() {
  /* do nothing */
  delay(1000);
}

void requestJSON(String url) {
  Serial.print("Requesting ");
  Serial.println(url);
  HTTPClient http;

  if (!http.begin(url)) {
    Serial.println("Cannot initiate request");
    return;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Status code: ");
    Serial.println(httpCode);
    if (httpCode < 0) {
      return;
    }
  }

  // Parse JSON
  DynamicJsonDocument doc(1024 * 10);
  DeserializationError error = deserializeJson(doc, http.getStream());
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.print(error.f_str());
  } else {
    serializeJsonPretty(doc, Serial);
  }

  // Serial.print("---->>>>");
  // Serial.println(http.getStream());


  http.end();
  Serial.println();
}

void GET_(String repoUrl) {
  String payload;
  HTTPClient http;
  http.begin(repoUrl);
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      payload = http.getString().substring(22);
    } else {
      Serial.println("Wrong HTTP Response: " + httpCode);
      return;
    }
  } else {
    Serial.println("Wrong HTTP Response: " + httpCode);
    return;
  }

  http.begin(payload);
  httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      payload = http.getString().substring(22);
    } else {
      Serial.println("Wrong HTTP Response: " + httpCode);
      return;
    }
  } else {
    Serial.println("Wrong HTTP Response: " + httpCode);
    return;
  }

  String fw = http.getString();
  Serial.println(fw);
}

void test_(String url) {
  HTTPClient http;
  http.begin(url);

  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    Serial.print("HTTP ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println();
    Serial.println(payload);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    Serial.println(":-(");
  }
}
