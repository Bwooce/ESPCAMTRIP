/**
 * GENERATED FILE - DO NOT EDIT
 * Generated on: 2025-12-02T23:27:44.629774
 * Source: YAML service definitions
 * Services: 122
 */

#ifndef NTRIP_GENERATED_SERVICES_H
#define NTRIP_GENERATED_SERVICES_H

#include "ntrip_atlas.h"

/**
 * Get generated service database
 * @param count Output parameter for service count
 * @return Pointer to service array
 */
const ntrip_service_compact_t* get_generated_services(size_t* count);

/**
 * Get provider name by index
 * @param provider_index Provider index from service
 * @return Provider name string
 */
const char* get_provider_name(uint8_t provider_index);

#endif // NTRIP_GENERATED_SERVICES_H