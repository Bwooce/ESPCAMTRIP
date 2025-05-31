#include "wifi_manager.h"
#include "config.h"
#include "system_state.h"

// Static member definitions
bool WiFiManager::connected = false;
unsigned long WiFiManager::connectionStartTime = 0;
uint32_t WiFiManager::disconnectCount = 0;
bool WiFiManager::timeSynchronized = false;

bool WiFiManager::connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(Config::wifi.ssid);
  
  // Disconnect if already connected
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
    delay(100);
  }
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Disable WiFi sleep for better performance
  
  // Start connection
  WiFi.begin(Config::wifi.ssid, Config::wifi.password);
  
  // Wait for connection
  unsigned long startAttempt = millis();
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttempt > Config::wifi.CONNECTION_TIMEOUT) {
      Serial.println("\nWiFi connection timeout!");
      connected = false;
      return false;
    }
    
    delay(500);
    Serial.print(".");
    digitalWrite(Config::pins.LED_STATUS_PIN, !digitalRead(Config::pins.LED_STATUS_PIN));
    attempts++;
    
    // Every 10 attempts, print status
    if (attempts % 10 == 0) {
      Serial.printf("\nWiFi Status: %d, RSSI: %d\n", WiFi.status(), WiFi.RSSI());
    }
  }
  
  // Connection successful
  connected = true;
  connectionStartTime = millis();
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
  Serial.print("Signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  
  digitalWrite(Config::pins.LED_STATUS_PIN, HIGH);
  
  return true;
}

void WiFiManager::disconnectWiFi() {
  if (connected) {
    WiFi.disconnect(true);
    connected = false;
    Serial.println("WiFi disconnected");
  }
}

bool WiFiManager::isConnected() {
  // Update connection status
  bool currentStatus = (WiFi.status() == WL_CONNECTED);
  
  // Handle state changes
  if (connected && !currentStatus) {
    handleDisconnect();
  } else if (!connected && currentStatus) {
    connected = true;
    connectionStartTime = millis();
  }
  
  return connected;
}

int WiFiManager::getRSSI() {
  if (connected) {
    return WiFi.RSSI();
  }
  return -100; // Return very weak signal if not connected
}

String WiFiManager::getIPAddress() {
  if (connected) {
    return WiFi.localIP().toString();
  }
  return "0.0.0.0";
}

bool WiFiManager::initializeTime() {
  Serial.println("Initializing time from NTP...");
  
  // Configure NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  
  // Wait for time to be set
  if (waitForTimeSync(10000)) { // 10 second timeout
    timeSynchronized = true;
    
    // Print current time
    struct tm timeinfo = {0}; // Initialized
    if (getLocalTime(&timeinfo)) {
      Serial.print("Time synchronized: ");
      Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
    }
    
    return true;
  }
  
  Serial.println("Time synchronization failed!");
  timeSynchronized = false;
  return false;
}

bool WiFiManager::isTimeSynchronized() {
  if (!timeSynchronized) {
    return false;
  }
  
  // Check if time is still valid (year > 2020)
  time_t now;
  time(&now);
  struct tm timeinfo = {0}; // Initialized
  localtime_r(&now, &timeinfo);
  
  return (timeinfo.tm_year + 1900) > 2020;
}

String WiFiManager::getCurrentTimestamp() {
  if (!isTimeSynchronized()) {
    return "TIME_NOT_SET";
  }
  
  time_t now;
  time(&now);
  struct tm timeinfo = {0}; // Initialized
  localtime_r(&now, &timeinfo);
  
  char timestamp[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  return String(timestamp);
}

void WiFiManager::checkConnection() {
  if (!isConnected()) {
    Serial.println("WiFi connection lost, attempting reconnect...");
    
    // Update system state
    SystemState::updateActivity();
    
    // Try to reconnect
    if (connectWiFi()) {
      Serial.println("WiFi reconnected successfully");
      
      // Re-sync time if needed
      if (!isTimeSynchronized()) {
        initializeTime();
      }
    } else {
      Serial.println("WiFi reconnection failed");
    }
  }
}

uint32_t WiFiManager::getConnectionUptime() {
  if (connected) {
    return (millis() - connectionStartTime) / 1000; // Return seconds
  }
  return 0;
}

uint32_t WiFiManager::getDisconnectCount() {
  return disconnectCount;
}

void WiFiManager::handleDisconnect() {
  connected = false;
  disconnectCount++;
  
  Serial.println("WiFi connection lost!");
  Serial.printf("Disconnect count: %u\n", disconnectCount); // %d -> %u
  
  digitalWrite(Config::pins.LED_STATUS_PIN, LOW);
  
  // Log disconnect event
  String logEntry = getCurrentTimestamp() + " - WiFi disconnected. Count: " + String(disconnectCount);
  // Note: Can't use StorageManager here due to circular dependency
  // Could implement a simple log queue instead
}

bool WiFiManager::waitForTimeSync(uint32_t timeoutMs) {
  unsigned long startTime = millis();
  
  Serial.print("Waiting for NTP time sync");
  
  while (millis() - startTime < timeoutMs) {
    time_t now;
    time(&now);
    struct tm timeinfo = {0}; // Initialized
    
    if (getLocalTime(&timeinfo, 100)) { // 100ms timeout for getLocalTime
      // Check if time is valid (year > 2020)
      if (timeinfo.tm_year + 1900 > 2020) {
        Serial.println(" OK");
        return true;
      }
    }
    
    Serial.print(".");
    delay(500);
  }
  
  Serial.println(" TIMEOUT");
  return false;
}
