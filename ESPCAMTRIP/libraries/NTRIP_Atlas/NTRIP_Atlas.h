#ifndef NTRIP_ATLAS_ARDUINO_H
#define NTRIP_ATLAS_ARDUINO_H

/*
 * NTRIP Atlas Arduino Library
 *
 * Global NTRIP service discovery and auto-selection for ESP32
 *
 * Usage:
 *   #include <NTRIP_Atlas.h>
 *
 *   ntrip_best_service_t service;
 *   if (ntrip_atlas_find_best(&service, latitude, longitude) == NTRIP_ATLAS_SUCCESS) {
 *     // Use service.server, service.port, service.mountpoint
 *   }
 */

#include "src/ntrip_atlas.h"

// ESP32 platform initialization - call this in setup()
bool ntrip_atlas_init_esp32();

#endif // NTRIP_ATLAS_ARDUINO_H