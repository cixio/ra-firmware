
#ifndef __M10PRIVATE_H
#define __M10PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lpclib.h"
#include "m10.h"
#include "gps.h"



/* A collection of all known packet types */
typedef __PACKED(struct {
    __PACKED(struct {
        uint8_t packetType;

        __PACKED(struct _M10_GpsBlock {
            uint8_t unknown1;
            uint8_t unknown2;
            int16_t speedEast;
            int16_t speedNorth;
            int16_t speedVertical;
            uint32_t tow;
            int32_t latitude;
            int32_t longitude;
            int32_t altitude;
            uint8_t reserved1[4];
            uint8_t visibleSats;
            uint8_t reserved2[1];
            uint16_t week;
            uint8_t prn[12];
        }) gps;

        __PACKED(struct _M10_ConfigBlock {
            uint8_t nnc0[16];
            uint8_t adc_temperature_range;
            uint16_t adc_temperature;
            uint8_t nnc1[4];
            uint16_t adc_vbat;
            uint8_t nnc2[22];
            uint8_t serial[5];
            uint8_t nn1[1];
        }) config;
    }) data;

    uint16_t crc;
}) M10_Packet;


typedef struct {
    double gpstime;
    LLA_Coordinate observerLLA;
    uint8_t visibleSats;
} M10_CookedGps;


typedef struct {
    float batteryVoltage;
    float temperature;
} M10_CookedMetrology;


/* Data that needs to be stored for every instance. */
typedef struct _M10_InstanceData {
    struct _M10_InstanceData *next;
    uint32_t id;
    char hashName[20];                          /* Hashable sonde name */
    uint32_t lastUpdated;
    float rxFrequencyMHz;
    uint16_t frameCounter;
    float rssi;
    uint64_t realTime;

    M10_CookedGps gps;
    M10_CookedMetrology metro;
} M10_InstanceData;


LPCLIB_Result _M10_processConfigBlock (
        const struct _M10_ConfigBlock *rawConfig,
        M10_InstanceData **instancePointer);

LPCLIB_Result _M10_processGpsBlock (
        const struct _M10_GpsBlock *rawGps,
        M10_CookedGps *cookedGps);

LPCLIB_Result _M10_processMetrologyBlock (
        const struct _M10_ConfigBlock *rawConfig,
        M10_CookedMetrology *cookedMetro);

/* Iterate through instances */
bool _M10_iterateInstance (M10_InstanceData **instance);

/* Remove an instance from the chain */
void _M10_deleteInstance (M10_InstanceData *instance);

#ifdef __cplusplus
}
#endif
#endif
