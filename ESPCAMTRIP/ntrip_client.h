#ifndef NTRIP_CLIENT_H
#define NTRIP_CLIENT_H

// Include compile-time flags from main sketch
// These are defined in ESPCAMTRIP.ino:
//   RTCM_OUTPUT_MAVLINK - Wrap RTCM in MAVLink (for flight controllers)
//   RTCM_OUTPUT_RAW     - Raw RTCM output (for direct GPS receiver connection)
//   RTCM_RAW_BAUD_RATE  - Baud rate for raw output mode

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>
#include <HardwareSerial.h>

// NTRIP Atlas integration for automatic service discovery
#ifdef NTRIP_ATLAS_ENABLED
#include "libraries/NTRIP_Atlas/NTRIP_Atlas.h"
#endif

// RTCM Message definitions
#define RTCM_PREAMBLE 0xD3
#define RTCM_MIN_LENGTH 6
#define RTCM_MAX_LENGTH 1029
#define RTCM_HEADER_LENGTH 3

// RTCM Message lists for different GNSS capabilities
#define L1_MSGLIST { 1005, 1006, 1074, 1075, 1084, 1085, 1094, 1095, 1114, 1115, 1019, 1020, 1042, 1044, 1046 }
#define L2_MSGLIST { 1076, 1077, 1086, 1087, 1096, 1097, 1116, 1117 }
#define L5_MSGLIST { 1126, 1127, 1128, 1129, 1136, 1137 }

// MAVLink configuration
#define MAVLINK_SYSTEM_ID 128
#define MAVLINK_COMPONENT_ID 191

class NtripClient {
public:
  // Statistics structure
  struct Statistics {
    uint32_t messagesReceived;
    uint32_t messagesValidated;
    uint32_t messagesForwarded;
    uint32_t connectionAttempts;
    uint32_t bytesReceived;
    unsigned long lastMessageTime;
    bool connected;
    bool rtcmDataReceived;
  };
  
  // Message type tracking
  struct MessageTypeStats {
    static const int MAX_TYPES = 20;
    int types[MAX_TYPES];
    int counts[MAX_TYPES];
    int numTypes;
    unsigned long lastLogTime;
    
    void reset();
    void addMessage(int type);
    void logStatistics();
  };
  
  // Public methods
  static bool init();
  static void startClient();
  static void stopClient();
  static bool isConnected();
  static bool isReceivingRTCM();
  static Statistics getStatistics();
  static void printStatistics();
  static const char* getRtcmMessageDescription(int messageType);

  // NTRIP Atlas automatic service discovery
  #ifdef NTRIP_ATLAS_ENABLED
  static bool tryAtlasDiscovery(double latitude, double longitude);
  #endif
  
  // Inter-task communication
  static void notifyActivity();
  static bool isIdle();
  
private:
  // CRC24Q calculation
  static unsigned int calculateCRC24Q(const uint8_t* buf, int len);
  
  // RTCM validation and processing
  static bool validateRtcmMessage(const uint8_t* buffer, int length);
  static int extractRtcmMessageType(const uint8_t* buffer, int length);
  static bool shouldRelayMessage(int messageType);
  static void processRtcmData(const uint8_t* buffer, size_t size);
  
  // Network functions
  static bool connectToNtrip();
  static void sendGgaToNtrip();
  static String createAuthHeader();
  
  // Output functions
  static void sendMavLinkHeartbeat();
  static void sendMavLinkRTCM(uint8_t* msg, uint16_t msglen);
  static void sendRawRTCM(uint8_t* msg, uint16_t msglen);
  
  // State variables
  static HardwareSerial mavlinkSerial;
  static WiFiClientSecure secureClient;
  static WiFiClient standardClient;
  static Statistics stats;
  static MessageTypeStats msgStats;
  static unsigned long lastGgaTime;
  static unsigned long lastActivity;
  static int reconnectCount;
  static bool clientRunning;
  
  // RTCM buffer for message assembly
  static uint8_t rtcmBuffer[RTCM_MAX_LENGTH + RTCM_HEADER_LENGTH + 3];
  static volatile int rtcmBufferIndex;
  static volatile bool inMessage;
  
  // Thread safety
  static portMUX_TYPE ntripMux;
  static void enterCritical();
  static void exitCritical();
  
  // CRC24Q table
  static const unsigned int crc24qtab[256];
  
  // Friend function for task
  friend void ntripClientTask(void* parameter);
};

// Task function (needs to be outside class for FreeRTOS)
void ntripClientTask(void* parameter);

#endif // NTRIP_CLIENT_H