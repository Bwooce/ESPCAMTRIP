#include "ntrip_client.h"
#include "config.h"
#include "system_state.h"
#include <base64.h>
#include <esp_task_wdt.h>

// NTRIP Atlas integration
#ifdef NTRIP_ATLAS_ENABLED
#include "libraries/NTRIP_Atlas/NTRIP_Atlas.h"
#endif

// MAVLink support - official library for proper wire-format
// Note: Temporarily disabled until MAVLink headers are properly generated
#ifdef RTCM_OUTPUT_MAVLINK_DISABLED_DISABLED
#include <MAVLink.h>
#endif

// Default to MAVLink mode if neither is defined (library now available)
#if !defined(RTCM_OUTPUT_MAVLINK) && !defined(RTCM_OUTPUT_RAW)
#define RTCM_OUTPUT_MAVLINK
#endif

// Default raw baud rate if not specified
#ifndef RTCM_RAW_BAUD_RATE
#define RTCM_RAW_BAUD_RATE 115200
#endif

// Static member definitions
HardwareSerial NtripClient::mavlinkSerial(2);
WiFiClientSecure NtripClient::secureClient;
WiFiClient NtripClient::standardClient;
NtripClient::Statistics NtripClient::stats = {0};
NtripClient::MessageTypeStats NtripClient::msgStats;
unsigned long NtripClient::lastGgaTime = 0;
unsigned long NtripClient::lastActivity = 0;
int NtripClient::reconnectCount = 0;
bool NtripClient::clientRunning = false;
uint8_t NtripClient::rtcmBuffer[RTCM_MAX_LENGTH + RTCM_HEADER_LENGTH + 3];
volatile int NtripClient::rtcmBufferIndex = 0;
volatile bool NtripClient::inMessage = false;
portMUX_TYPE NtripClient::ntripMux = portMUX_INITIALIZER_UNLOCKED;

// CRC24Q table
const unsigned int NtripClient::crc24qtab[256] = {
  0x000000, 0x864CFB, 0x8AD50D, 0x0C99F6, 0x93E6E1, 0x15AA1A, 0x1933EC, 0x9F7F17,
  0xA18139, 0x27CDC2, 0x2B5434, 0xAD18CF, 0x3267D8, 0xB42B23, 0xB8B2D5, 0x3EFE2E,
  0xC54E89, 0x430272, 0x4F9B84, 0xC9D77F, 0x56A868, 0xD0E493, 0xDC7D65, 0x5A319E,
  0x64CFB0, 0xE2834B, 0xEE1ABD, 0x685646, 0xF72951, 0x7165AA, 0x7DFC5C, 0xFBB0A7,
  0x0CD1E9, 0x8A9D12, 0x8604E4, 0x00481F, 0x9F3708, 0x197BF3, 0x15E205, 0x93AEFE,
  0xAD50D0, 0x2B1C2B, 0x2785DD, 0xA1C926, 0x3EB631, 0xB8FACA, 0xB4633C, 0x322FC7,
  0xC99F60, 0x4FD39B, 0x434A6D, 0xC50696, 0x5A7981, 0xDC357A, 0xD0AC8C, 0x56E077,
  0x681E59, 0xEE52A2, 0xE2CB54, 0x6487AF, 0xFBF8B8, 0x7DB443, 0x712DB5, 0xF7614E,
  0x19A3D2, 0x9FEF29, 0x9376DF, 0x153A24, 0x8A4533, 0x0C09C8, 0x00903E, 0x86DCC5,
  0xB822EB, 0x3E6E10, 0x32F7E6, 0xB4BB1D, 0x2BC40A, 0xAD88F1, 0xA11107, 0x275DFC,
  0xDCED5B, 0x5AA1A0, 0x563856, 0xD074AD, 0x4F0BBA, 0xC94741, 0xC5DEB7, 0x43924C,
  0x7D6C62, 0xFB2099, 0xF7B96F, 0x71F594, 0xEE8A83, 0x68C678, 0x645F8E, 0xE21375,
  0x15723B, 0x933EC0, 0x9FA736, 0x19EBCD, 0x8694DA, 0x00D821, 0x0C41D7, 0x8A0D2C,
  0xB4F302, 0x32BFF9, 0x3E260F, 0xB86AF4, 0x2715E3, 0xA15918, 0xADC0EE, 0x2B8C15,
  0xD03CB2, 0x567049, 0x5AE9BF, 0xDCA544, 0x43DA53, 0xC596A8, 0xC90F5E, 0x4F43A5,
  0x71BD8B, 0xF7F170, 0xFB6886, 0x7D247D, 0xE25B6A, 0x641791, 0x688E67, 0xEEC29C,
  0x3347A4, 0xB50B5F, 0xB992A9, 0x3FDE52, 0xA0A145, 0x26EDBE, 0x2A7448, 0xAC38B3,
  0x92C69D, 0x148A66, 0x181390, 0x9E5F6B, 0x01207C, 0x876C87, 0x8BF571, 0x0DB98A,
  0xF6092D, 0x7045D6, 0x7CDC20, 0xFA90DB, 0x65EFCC, 0xE3A337, 0xEF3AC1, 0x69763A,
  0x578814, 0xD1C4EF, 0xDD5D19, 0x5B11E2, 0xC46EF5, 0x42220E, 0x4EBBF8, 0xC8F703,
  0x3F964D, 0xB9DAB6, 0xB54340, 0x330FBB, 0xAC70AC, 0x2A3C57, 0x26A5A1, 0xA0E95A,
  0x9E1774, 0x185B8F, 0x14C279, 0x928E82, 0x0DF195, 0x8BBD6E, 0x872498, 0x016863,
  0xFAD8C4, 0x7C943F, 0x700DC9, 0xF64132, 0x693E25, 0xEF72DE, 0xE3EB28, 0x65A7D3,
  0x5B59FD, 0xDD1506, 0xD18CF0, 0x57C00B, 0xC8BF1C, 0x4EF3E7, 0x426A11, 0xC426EA,
  0x2AE476, 0xACA88D, 0xA0317B, 0x267D80, 0xB90297, 0x3F4E6C, 0x33D79A, 0xB59B61,
  0x8B654F, 0x0D29B4, 0x01B042, 0x87FCB9, 0x1883AE, 0x9ECF55, 0x9256A3, 0x141A58,
  0xEFAAFF, 0x69E604, 0x657FF2, 0xE33309, 0x7C4C1E, 0xFA00E5, 0xF69913, 0x70D5E8,
  0x4E2BC6, 0xC8673D, 0xC4FECB, 0x42B230, 0xDDCD27, 0x5B81DC, 0x57182A, 0xD154D1,
  0x26359F, 0xA07964, 0xACE092, 0x2AAC69, 0xB5D37E, 0x339F85, 0x3F0673, 0xB94A88,
  0x87B4A6, 0x01F85D, 0x0D61AB, 0x8B2D50, 0x145247, 0x921EBC, 0x9E874A, 0x18CBB1,
  0xE37B16, 0x6537ED, 0x69AE1B, 0xEFE2E0, 0x709DF7, 0xF6D10C, 0xFA48FA, 0x7C0401,
  0x42FA2F, 0xC4B6D4, 0xC82F22, 0x4E63D9, 0xD11CCE, 0x575035, 0x5BC9C3, 0xDD8538
};

// Root certificate for NTRIP caster
const char* rootCACertificate =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
  "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
  "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
  "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
  "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
  "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
  "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
  "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
  "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
  "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
  "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
  "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
  "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
  "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
  "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
  "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
  "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
  "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
  "-----END CERTIFICATE-----";

bool NtripClient::init() {
  Serial.println("Initializing NTRIP client...");

  // Initialize GPS Position Manager
  if (!GPSPositionManager::init()) {
    Serial.println("Failed to initialize GPS Position Manager");
    return false;
  }

  // Initialize UART for RTCM output
#ifdef RTCM_OUTPUT_RAW
  Serial.printf("RTCM output mode: RAW (direct to GPS receiver) at %d baud\n", RTCM_RAW_BAUD_RATE);
  mavlinkSerial.begin(RTCM_RAW_BAUD_RATE, SERIAL_8N1, Config::pins.RTCM_UART_RX, Config::pins.RTCM_UART_TX);
#else
  Serial.println("RTCM output mode: MAVLink wrapped");
  mavlinkSerial.begin(115200, SERIAL_8N1, Config::pins.RTCM_UART_RX, Config::pins.RTCM_UART_TX);
#endif
  
  // Reset statistics
  memset(&stats, 0, sizeof(stats));
  msgStats.reset();
  
  // Configure SSL if needed
  if (Config::ntrip.use_ssl) {
    secureClient.setCACert(rootCACertificate);
    // Optional: secureClient.setInsecure(); // For testing
  }
  
  enterCritical();
  clientRunning = false;
  exitCritical();
  Serial.println("NTRIP client initialized");
  return true;
}

void NtripClient::startClient() {
  enterCritical();
  clientRunning = true;
  stats.connected = false;
  stats.rtcmDataReceived = false;
  exitCritical();
}

void NtripClient::stopClient() {
  enterCritical();
  clientRunning = false;
  stats.connected = false;
  exitCritical();
  
  if (Config::ntrip.use_ssl) {
    secureClient.stop();
  } else {
    standardClient.stop();
  }
}

bool NtripClient::isConnected() {
  enterCritical();
  bool connected = stats.connected;
  exitCritical();
  return connected;
}

bool NtripClient::isReceivingRTCM() {
  enterCritical();
  bool receiving = stats.rtcmDataReceived && 
                   (millis() - stats.lastMessageTime < Config::ntrip.RTCM_TIMEOUT);
  exitCritical();
  return receiving;
}

NtripClient::Statistics NtripClient::getStatistics() {
  enterCritical();
  Statistics localStats = stats;
  exitCritical();
  return localStats;
}

void NtripClient::printStatistics() {
  Statistics localStats = getStatistics();
  
  Serial.println("NTRIP Statistics:");
  Serial.printf("  Connected: %s\n", localStats.connected ? "Yes" : "No");
  Serial.printf("  Messages received: %u\n", localStats.messagesReceived);
  Serial.printf("  Messages validated: %u\n", localStats.messagesValidated);
  Serial.printf("  Messages forwarded: %u\n", localStats.messagesForwarded);
  Serial.printf("  Bytes received: %u\n", localStats.bytesReceived);
  Serial.printf("  Connection attempts: %u\n", localStats.connectionAttempts);
  
  if (localStats.lastMessageTime > 0) {
    Serial.printf("  Last message: %lu ms ago\n", millis() - localStats.lastMessageTime);
  }
}

void NtripClient::notifyActivity() {
  enterCritical();
  lastActivity = millis();
  exitCritical();
  SystemState::updateNtripActivity();
}

bool NtripClient::isIdle() {
  enterCritical();
  unsigned long activity = lastActivity;
  exitCritical();
  return (millis() - activity > 5000);
}

unsigned int NtripClient::calculateCRC24Q(const uint8_t* buf, int len) {
  unsigned int crc = 0;
  for (int i = 0; i < len; i++) {
    crc = ((crc << 8) & 0xFFFFFF) ^ crc24qtab[(crc >> 16) ^ buf[i]];
  }
  return crc & 0xFFFFFF;
}

bool NtripClient::validateRtcmMessage(const uint8_t* buffer, int length) {
  if (length < RTCM_MIN_LENGTH || buffer[0] != RTCM_PREAMBLE) {
    return false;
  }
  
  int messageLength = ((buffer[1] & 0x03) << 8) | buffer[2];
  
  if (messageLength > RTCM_MAX_LENGTH || 
      length < messageLength + RTCM_HEADER_LENGTH + 3) {
    return false;
  }
  
  unsigned int calculatedCrc = calculateCRC24Q(buffer, messageLength + RTCM_HEADER_LENGTH);
  unsigned int receivedCrc = (buffer[messageLength + RTCM_HEADER_LENGTH] << 16) | 
                            (buffer[messageLength + RTCM_HEADER_LENGTH + 1] << 8) | 
                            buffer[messageLength + RTCM_HEADER_LENGTH + 2];
  
  return calculatedCrc == receivedCrc;
}

int NtripClient::extractRtcmMessageType(const uint8_t* buffer, int length) {
  if (length < 6 || buffer[0] != RTCM_PREAMBLE) {
    return -1;
  }
  
  return ((buffer[3] << 4) | ((buffer[4] >> 4) & 0x0F));
}

bool NtripClient::shouldRelayMessage(int messageType) {
  const uint16_t msglist[] = L1_MSGLIST; // Using L1 for M9N
  
  for (size_t i = 0; i < sizeof(msglist) / sizeof(msglist[0]); i++) {
    if (msglist[i] == messageType) {
      return true;
    }
  }
  return false;
}

void NtripClient::processRtcmData(const uint8_t* buffer, size_t size) {
  enterCritical();
  stats.bytesReceived += size;
  exitCritical();
  
  for (size_t i = 0; i < size; i++) {
    uint8_t byte = buffer[i];
    
    enterCritical();
    bool wasInMessage = inMessage;
    int currentIndex = rtcmBufferIndex;
    
    if (!inMessage && byte == RTCM_PREAMBLE) {
      rtcmBufferIndex = 0;
      inMessage = true;
      wasInMessage = true;
      currentIndex = 0;
    }
    
    if (wasInMessage) {
      if (currentIndex < sizeof(rtcmBuffer)) {
        rtcmBuffer[currentIndex] = byte;
        rtcmBufferIndex = currentIndex + 1;
        
        if (rtcmBufferIndex >= RTCM_MIN_LENGTH) {
          int messageLength = ((rtcmBuffer[1] & 0x03) << 8) | rtcmBuffer[2];
          
          if (messageLength > RTCM_MAX_LENGTH) {
            inMessage = false;
            exitCritical();
            continue;
          }
          
          if (rtcmBufferIndex >= messageLength + RTCM_HEADER_LENGTH + 3) {
            // Message complete - copy to local buffer for processing
            uint8_t localBuffer[RTCM_MAX_LENGTH + RTCM_HEADER_LENGTH + 3];
            int localBufferSize = rtcmBufferIndex;
            memcpy(localBuffer, rtcmBuffer, localBufferSize);
            
            stats.messagesReceived++;
            inMessage = false;
            exitCritical();
            
            // Process outside critical section
            if (validateRtcmMessage(localBuffer, localBufferSize)) {
              int messageType = extractRtcmMessageType(localBuffer, localBufferSize);
              
              msgStats.addMessage(messageType);
              
              if (shouldRelayMessage(messageType)) {
#ifdef RTCM_OUTPUT_RAW
                sendRawRTCM(localBuffer, localBufferSize);
#else
                sendMavLinkRTCM(localBuffer, localBufferSize);
#endif

                enterCritical();
                stats.messagesValidated++;
                stats.messagesForwarded++;
                stats.rtcmDataReceived = true;
                stats.lastMessageTime = millis();
                exitCritical();
              }
            }
            continue;
          }
        }
      } else {
        inMessage = false;
      }
    }
    exitCritical();
  }
}

bool NtripClient::connectToNtrip() {
  Serial.println("Connecting to NTRIP caster...");
  stats.connectionAttempts++;

  #ifdef NTRIP_ATLAS_ENABLED
  // Primary: Use NTRIP Atlas intelligent service discovery
  if (Config::ntrip.server.isEmpty() || stats.connectionAttempts == 1) {
    Serial.println("Using NTRIP Atlas for intelligent service discovery...");

    // Try discovery with live GPS position first, fallback to static coordinates
    if (tryAtlasDiscoveryWithLivePosition()) {
      Serial.println("Atlas found optimal service using live GPS position, attempting connection...");
    } else if (tryAtlasDiscovery(-33.7986, 151.1722)) {
      Serial.println("Atlas found optimal service using static coordinates, attempting connection...");
    } else {
      Serial.println("Atlas discovery failed, using fallback config if available");
      // Fall through to hardcoded config if Atlas fails
    }
  }
  #endif

  WiFiClient* client = Config::ntrip.use_ssl ?
                      (WiFiClient*)&secureClient :
                      (WiFiClient*)&standardClient;

  if (!client->connect(Config::ntrip.server.c_str(), Config::ntrip.port)) {
    Serial.println("Connection to NTRIP caster failed");
    return false;
  }

  client->setTimeout(15000);

  // Send HTTP request
  String request = "GET /" + String(Config::ntrip.mountpoint) + " HTTP/1.1\r\n";
  request += "Host: " + String(Config::ntrip.server) + "\r\n";
  request += "User-Agent: NTRIP ESP32Client/2.0\r\n";
  request += createAuthHeader() + "\r\n";
  request += "Connection: keep-alive\r\n\r\n";

  client->print(request);

  // Wait for response
  unsigned long timeout = millis();
  while (client->available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println("Client timeout!");
      client->stop();
      return false;
    }
    delay(100);
  }

  // Check response
  String responseLine = client->readStringUntil('\n');
  Serial.println("Response: " + responseLine);

  if (responseLine.indexOf("200 OK") > 0 || responseLine.indexOf("ICY 200 OK") > 0) {
    // Skip headers
    while (client->available()) {
      String line = client->readStringUntil('\n');
      if (line == "\r") break;
    }

    stats.connected = true;
    reconnectCount = 0;
    Serial.println("Connected to NTRIP caster successfully");
    return true;
  }

  Serial.println("Invalid response from NTRIP caster");
  client->stop();
  return false;
}

void NtripClient::sendGgaToNtrip() {
  if (millis() - lastGgaTime > Config::ntrip.GGA_INTERVAL) {
    WiFiClient* client = Config::ntrip.use_ssl ?
                        (WiFiClient*)&secureClient :
                        (WiFiClient*)&standardClient;

    if (client->connected()) {
      // Generate dynamic GGA message based on live GPS position
      String gga = GPSPositionManager::generateGGAMessage();
      client->println(gga);
      client->clear();
      lastGgaTime = millis();

      // Debug output every 10 GGA sends (reduce spam)
      static int ggaCount = 0;
      if (++ggaCount % 10 == 0) {
        GPSQuality qual = GPSPositionManager::getPositionQuality();
        Serial.printf("GGA sent: %s (Quality: %s)\n", gga.c_str(), qual.status.c_str());
      }
    }
  }
}

String NtripClient::createAuthHeader() {
  String credentials = String(Config::ntrip.username) + ":" + String(Config::ntrip.password);
  return "Authorization: Basic " + base64::encode(credentials);
}

#ifdef NTRIP_ATLAS_ENABLED
bool NtripClient::tryAtlasDiscovery(double latitude, double longitude) {
  Serial.println("Trying NTRIP Atlas service discovery...");

  // Initialize NTRIP Atlas for ESP32
  if (!ntrip_atlas_init_esp32()) {
    Serial.println("Failed to initialize NTRIP Atlas");
    return false;
  }

  // Find best service for current location
  ntrip_best_service_t service;
  if (ntrip_atlas_find_best(&service, latitude, longitude) == NTRIP_ATLAS_SUCCESS) {
    // Update config with discovered service
    Serial.printf("Atlas discovered: %s:%d/%s (%.1fkm away)\n",
                  service.server, service.port, service.mountpoint, service.distance_km);

    Config::ntrip.server = service.server;
    Config::ntrip.port = service.port;
    Config::ntrip.mountpoint = service.mountpoint;
    Config::ntrip.use_ssl = false; // Most NTRIP services use plain HTTP

    // Note: User will need to configure credentials manually
    // Could be enhanced to check for free services first
    Serial.println("Atlas discovery successful! Configure credentials if needed.");
    return true;
  }

  Serial.println("No suitable NTRIP service found via Atlas");
  return false;
}

bool NtripClient::tryAtlasDiscoveryWithLivePosition() {
  if (!GPSPositionManager::hasValidPosition()) {
    Serial.println("No live GPS position available for Atlas discovery");
    return false;
  }

  GPSPosition pos = GPSPositionManager::getCurrentPosition();
  Serial.printf("Using live GPS position: %.6f, %.6f for Atlas discovery\n",
                pos.latitude, pos.longitude);

  return tryAtlasDiscovery(pos.latitude, pos.longitude);
}
#endif

void NtripClient::processIncomingGPSData() {
  // Process incoming data from GPS receiver or flight controller
  if (mavlinkSerial.available()) {
    while (mavlinkSerial.available()) {
      char c = mavlinkSerial.read();

      #ifdef RTCM_OUTPUT_RAW
      // In raw mode, parse incoming NMEA from GPS receiver
      static String nmeaBuffer = "";
      if (c == '\n') {
        if (nmeaBuffer.length() > 0) {
          GPSPositionManager::updateFromNMEA(nmeaBuffer);
          nmeaBuffer = "";
        }
      } else if (c != '\r') {
        nmeaBuffer += c;
        if (nmeaBuffer.length() > 200) {
          nmeaBuffer = ""; // Prevent buffer overflow
        }
      }
      #else
      // In MAVLink mode, parse incoming MAVLink messages from flight controller
      static uint8_t mavlinkBuffer[256];
      static size_t bufferPos = 0;

      mavlinkBuffer[bufferPos++] = c;
      if (bufferPos >= sizeof(mavlinkBuffer)) {
        bufferPos = 0; // Reset on overflow
      }

      // Simple MAVLink processing - look for complete messages
      if (bufferPos >= 8) { // Minimum MAVLink message size
        GPSPositionManager::updateFromMAVLink(mavlinkBuffer, bufferPos);
        bufferPos = 0; // Reset buffer after processing
      }
      #endif
    }
  }
}

bool NtripClient::updatePositionFromGPS() {
  processIncomingGPSData();
  return GPSPositionManager::hasValidPosition();
}

void NtripClient::printGPSStatus() {
  GPSPosition pos = GPSPositionManager::getCurrentPosition();
  GPSQuality qual = GPSPositionManager::getPositionQuality();

  Serial.println("\n=== GPS Status ===");
  if (pos.valid) {
    Serial.printf("Position: %.6f, %.6f @ %.1fm\n",
                  pos.latitude, pos.longitude, pos.altitude);
    Serial.printf("Fix Type: %d, Satellites: %d, HDOP: %.1f\n",
                  pos.fixType, pos.satellites, pos.hdop);
    Serial.printf("Quality: %s (±%.2fm)\n", qual.status.c_str(), qual.accuracy);
    Serial.printf("Age: %lums\n", millis() - pos.timestamp);
  } else {
    Serial.println("No valid GPS position");
  }
  Serial.printf("Messages: %lu total, %lu valid\n",
                GPSPositionManager::getTotalMessages(),
                GPSPositionManager::getValidMessages());
  Serial.println("==================");
}

// Raw RTCM output - sends RTCM3 binary directly to UART
// Use this for direct connection to u-blox ZED-F9P, NEO-M8P, etc.
void NtripClient::sendRawRTCM(uint8_t* msg, uint16_t msglen) {
  mavlinkSerial.write(msg, msglen);
}

#ifdef RTCM_OUTPUT_MAVLINK_DISABLED
// Simple CRC16 calculation for MAVLink
uint16_t calculateCRC16(uint8_t* data, uint16_t length, uint16_t crc_extra = 0) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < length; i++) {
    crc ^= data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
    }
  }
  if (crc_extra > 0) {
    crc ^= crc_extra << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
    }
  }
  return crc;
}

void NtripClient::sendMavLinkHeartbeat() {
  static uint32_t lastSent = 0;
  static uint8_t seq = 0;
  if (millis() - lastSent < 1000) return;

  // MAVLink heartbeat packet (minimal implementation)
  uint8_t packet[14]; // Header(6) + payload(6) + CRC(2)
  packet[0] = MAVLINK_STX;      // Magic byte
  packet[1] = 6;                   // Payload length
  packet[2] = seq++;               // Sequence
  packet[3] = 1;                   // System ID
  packet[4] = 1;                   // Component ID
  packet[5] = MAVLINK_MSG_ID_HEARTBEAT; // Message ID

  // Heartbeat payload (6 bytes)
  packet[6] = 18;                  // Type: onboard controller
  packet[7] = 0;                   // Autopilot: generic
  packet[8] = 0;                   // Base mode
  packet[9] = 0;                   // Custom mode (4 bytes)
  packet[10] = 0;
  packet[11] = 0;
  packet[12] = 0;
  packet[13] = 3;                  // System status: standby

  // Calculate CRC (simplified, no CRC_EXTRA for now)
  uint16_t crc = calculateCRC16(&packet[1], 12);
  mavlinkSerial.write(packet, 14);
  mavlinkSerial.write((uint8_t*)&crc, 2);

  lastSent = millis();
}

void NtripClient::sendMavLinkRTCM(uint8_t* msg, uint16_t msglen) {
  static uint8_t seq = 0;
  const uint8_t MAX_RTCM_DATA_LEN = 180; // GPS_RTCM_DATA field size

  if (msglen == 0 || msglen > (4 * MAX_RTCM_DATA_LEN)) {
    return; // Invalid size
  }

  // Simple implementation: send as single packet if small enough
  if (msglen <= MAX_RTCM_DATA_LEN) {
    uint8_t packet[8 + MAX_RTCM_DATA_LEN]; // Header(6) + payload(2+data) + CRC(2)

    // MAVLink GPS_RTCM_DATA packet header
    packet[0] = MAVLINK_STX;              // Magic byte
    packet[1] = 2 + msglen;                  // Payload length (flags + len + data)
    packet[2] = seq++;                       // Sequence
    packet[3] = 1;                           // System ID
    packet[4] = 1;                           // Component ID
    packet[5] = MAVLINK_MSG_ID_GPS_RTCM_DATA;// Message ID

    // GPS_RTCM_DATA payload
    packet[6] = 0;                           // Flags (no fragmentation)
    packet[7] = msglen;                      // Length
    memcpy(&packet[8], msg, msglen);         // RTCM data

    // Calculate and append CRC
    uint16_t crc = calculateCRC16(&packet[1], 7 + msglen);
    packet[8 + msglen] = crc & 0xFF;         // CRC low byte
    packet[9 + msglen] = (crc >> 8) & 0xFF;  // CRC high byte

    mavlinkSerial.write(packet, 10 + msglen);
  }
  // For larger messages, we could implement fragmentation, but for now just truncate
  // Most RTCM3 correction messages are under 180 bytes anyway
}
#else
// Stub functions when MAVLink is not used
void NtripClient::sendMavLinkHeartbeat() {}
void NtripClient::sendMavLinkRTCM(uint8_t* msg, uint16_t msglen) {
  (void)msg;
  (void)msglen;
}
#endif // RTCM_OUTPUT_MAVLINK

const char* NtripClient::getRtcmMessageDescription(int messageType) {
  switch (messageType) {
    // MSM Messages
    case 1074: return "GPS MSM4";
    case 1075: return "GPS MSM5";
    case 1084: return "GLONASS MSM4";
    case 1085: return "GLONASS MSM5";
    case 1094: return "Galileo MSM4";
    case 1095: return "Galileo MSM5";
    case 1114: return "QZSS MSM4";
    case 1115: return "QZSS MSM5";
    case 1124: return "BeiDou MSM4";
    case 1125: return "BeiDou MSM5";
    
    // RTK Messages
    case 1005: return "Stationary RTK Reference Station ARP";
    case 1006: return "Stationary RTK Reference Station ARP with Height";
    case 1019: return "GPS Ephemeris";
    case 1020: return "GLONASS Ephemeris";
    case 1042: return "BeiDou Ephemeris";
    case 1044: return "QZSS Ephemeris";
    case 1046: return "Galileo I/NAV Ephemeris";
    
    default: return "Unknown RTCM Message";
  }
}

// MessageTypeStats implementation
void NtripClient::MessageTypeStats::reset() {
  for (int i = 0; i < MAX_TYPES; i++) {
    types[i] = 0;
    counts[i] = 0;
  }
  numTypes = 0;
  lastLogTime = 0;
}

void NtripClient::MessageTypeStats::addMessage(int type) {
  if (type <= 0) return;
  
  for (int i = 0; i < numTypes; i++) {
    if (types[i] == type) {
      counts[i]++;
      return;
    }
  }
  
  if (numTypes < MAX_TYPES) {
    types[numTypes] = type;
    counts[numTypes] = 1;
    numTypes++;
  }
}

void NtripClient::MessageTypeStats::logStatistics() {
  unsigned long currentTime = millis();
  if (currentTime - lastLogTime < 30000 && lastLogTime != 0) {
    return;
  }
  
  lastLogTime = currentTime;
  
  if (numTypes > 0) {
    Serial.println("RTCM Message Statistics (last 30 seconds):");
    for (int i = 0; i < numTypes; i++) {
      Serial.printf("  Type %d (%s): %d messages\n", 
                    types[i], 
                    NtripClient::getRtcmMessageDescription(types[i]), 
                    counts[i]);
    }
    
    // Reset counts
    for (int i = 0; i < numTypes; i++) {
      counts[i] = 0;
    }
  }
}

// NTRIP Client Task
void ntripClientTask(void* parameter) {
  // Initialize watchdog
  esp_task_wdt_config_t wdtConfig;
  wdtConfig.timeout_ms = Config::timing.WATCHDOG_TIMEOUT;
  wdtConfig.idle_core_mask = (1 << 1);  // Core 1
  wdtConfig.trigger_panic = true;
  esp_task_wdt_init(&wdtConfig);
  esp_task_wdt_add(NULL);
  
  Serial.println("NTRIP client task started");
  NtripClient::init();
  NtripClient::startClient();
  
  bool connected = false;
  WiFiClient* client = nullptr;
  
  while (true) {
    esp_task_wdt_reset();
    
    // Send MAVLink heartbeat
    NtripClient::sendMavLinkHeartbeat();
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(Config::pins.LED_STATUS_PIN, LOW);
      connected = false;
      delay(1000);
      continue;
    }
    
    // Connect if not connected
    if (!connected) {
      if (NtripClient::connectToNtrip()) {
        connected = true;
        NtripClient::enterCritical();
        NtripClient::stats.connected = true;
        NtripClient::exitCritical();
        digitalWrite(Config::pins.LED_STATUS_PIN, HIGH);
        client = Config::ntrip.use_ssl ? 
                (WiFiClient*)&NtripClient::secureClient : 
                (WiFiClient*)&NtripClient::standardClient;
      } else {
        NtripClient::enterCritical();
        NtripClient::reconnectCount++;
        int currentReconnectCount = NtripClient::reconnectCount;
        NtripClient::exitCritical();
        if (currentReconnectCount >= Config::wifi.MAX_RECONNECT_ATTEMPTS) {
          Serial.println("Max NTRIP reconnect attempts reached");
          NtripClient::enterCritical();
          NtripClient::reconnectCount = 0;
          NtripClient::exitCritical();
        }
        delay(Config::wifi.RECONNECT_DELAY);
        continue;
      }
    }
    
    // Process incoming GPS data for live positioning
    NtripClient::processIncomingGPSData();

    // Handle connected state
    if (client) {
      // Send GGA periodically
      NtripClient::sendGgaToNtrip();

      // Check for data
      if (client->available()) {
        uint8_t buffer[1024];
        size_t size = client->available();
        size = client->read(buffer, (size < sizeof(buffer)) ? size : sizeof(buffer));
        
        if (size > 0) {
          NtripClient::processRtcmData(buffer, size);
          NtripClient::notifyActivity();
        }
      }
      
      // Check for timeout
      if (NtripClient::isReceivingRTCM() && 
          (millis() - NtripClient::stats.lastMessageTime > Config::ntrip.RTCM_TIMEOUT)) {
        Serial.println("RTCM timeout, reconnecting...");
        client->stop();
        connected = false;
        delay(1000);
        continue;
      }
      
      // Check connection
      if (!client->connected()) {
        Serial.println("NTRIP connection lost");
        connected = false;
        NtripClient::enterCritical();
        NtripClient::stats.connected = false;
        NtripClient::exitCritical();
        delay(1000);
        continue;
      }
    }
    
    // Log statistics periodically
    NtripClient::msgStats.logStatistics();
    
    // Small delay
    delay(10);
  }
}

void NtripClient::enterCritical() {
  portENTER_CRITICAL(&ntripMux);
}

void NtripClient::exitCritical() {
  portEXIT_CRITICAL(&ntripMux);
}

