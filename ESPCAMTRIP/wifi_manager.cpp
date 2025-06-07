#include "wifi_manager.h"
#include "config.h"
#include "system_state.h"

// Static member definitions
bool WiFiManager::connected = false;
unsigned long WiFiManager::connectionStartTime = 0;
uint32_t WiFiManager::disconnectCount = 0;
bool WiFiManager::timeSynchronized = false;
portMUX_TYPE WiFiManager::wifiMux = portMUX_INITIALIZER_UNLOCKED;

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
    if (isTimeElapsed(startAttempt, Config::wifi.CONNECTION_TIMEOUT)) {
      Serial.println("\nWiFi connection timeout!");
      enterCritical();
      connected = false;
      exitCritical();
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
  enterCritical();
  connected = true;
  connectionStartTime = millis();
  exitCritical();
  
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
  enterCritical();
  bool wasConnected = connected;
  if (connected) {
    connected = false;
  }
  exitCritical();
  
  if (wasConnected) {
    WiFi.disconnect(true);
    Serial.println("WiFi disconnected");
  }
}

bool WiFiManager::isConnected() {
  // Update connection status
  bool currentStatus = (WiFi.status() == WL_CONNECTED);
  bool result;
  
  enterCritical();
  // Handle state changes
  if (connected && !currentStatus) {
    connected = false;
    disconnectCount++;
    result = false;
  } else if (!connected && currentStatus) {
    connected = true;
    connectionStartTime = millis();
    result = true;
  } else {
    result = connected;
  }
  exitCritical();
  
  // Handle disconnect logging outside critical section
  if (connected != currentStatus && !currentStatus) {
    Serial.println("WiFi connection lost!");
    Serial.printf("Disconnect count: %u\n", disconnectCount);
    digitalWrite(Config::pins.LED_STATUS_PIN, LOW);
  }
  
  return result;
}

int WiFiManager::getRSSI() {
  enterCritical();
  bool isConn = connected;
  exitCritical();
  
  if (isConn) {
    return WiFi.RSSI();
  }
  return -100; // Return very weak signal if not connected
}

String WiFiManager::getIPAddress() {
  enterCritical();
  bool isConn = connected;
  exitCritical();
  
  if (isConn) {
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
    enterCritical();
    timeSynchronized = true;
    exitCritical();
    
    // Print current time
    struct tm timeinfo = {0}; // Initialized
    if (getLocalTime(&timeinfo)) {
      Serial.print("Time synchronized: ");
      Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
    }
    
    return true;
  }
  
  Serial.println("Time synchronization failed!");
  enterCritical();
  timeSynchronized = false;
  exitCritical();
  return false;
}

bool WiFiManager::isTimeSynchronized() {
  enterCritical();
  bool timeSync = timeSynchronized;
  exitCritical();
  
  if (!timeSync) {
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
  enterCritical();
  bool isConn = connected;
  unsigned long startTime = connectionStartTime;
  exitCritical();
  
  if (isConn) {
    return (millis() - startTime) / 1000; // Return seconds
  }
  return 0;
}

uint32_t WiFiManager::getDisconnectCount() {
  enterCritical();
  uint32_t count = disconnectCount;
  exitCritical();
  return count;
}

void WiFiManager::handleDisconnect() {
  // This function is now deprecated - disconnect handling moved to isConnected()
  // Left here for compatibility but should not be called
}

bool WiFiManager::waitForTimeSync(uint32_t timeoutMs) {
  unsigned long startTime = millis();
  
  Serial.print("Waiting for NTP time sync");
  
  while (!isTimeElapsed(startTime, timeoutMs)) {
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

void WiFiManager::enterCritical() {
  portENTER_CRITICAL(&wifiMux);
}

void WiFiManager::exitCritical() {
  portEXIT_CRITICAL(&wifiMux);
}

bool WiFiManager::isTimeElapsed(unsigned long startTime, unsigned long interval) {
  // Overflow-safe time comparison
  unsigned long currentTime = millis();
  return (currentTime - startTime) >= interval;
}
