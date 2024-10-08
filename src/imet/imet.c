
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lpclib.h"
#include "bsp.h"
#include "sys.h"
#include "imet.h"
#include "imetprivate.h"



/** Context */
typedef struct IMET_Context {
    IMET_InstanceData *instance;
    float rxFrequencyHz;
} IMET_Context;

static IMET_Context _imet;


/* Verify the packet checksum.
 * The last two payload bytes contain the CRC.
 */
static bool _IMET_doParityCheck (uint8_t *buffer, uint8_t length)
{
    CRC_Handle crc = LPCLIB_INVALID_HANDLE;
    CRC_Mode crcMode;
    uint16_t receivedCRC;
    bool result = false;

    crcMode = CRC_makeMode(
            CRC_POLY_CRCCCITT,
            CRC_DATAORDER_NORMAL,
            CRC_SUMORDER_NORMAL,
            CRC_DATAPOLARITY_NORMAL,
            CRC_SUMPOLARITY_NORMAL
            );
    if (CRC_open(crcMode, &crc) == LPCLIB_SUCCESS) {
        CRC_seed(crc, 0xDCBD);  /* Seed=0x1D0F plus SOH byte (0x01) */
        CRC_write(crc, buffer, length - 2, NULL, NULL);
        receivedCRC = (buffer[length-2] << 8) | buffer[length-1];
        result = receivedCRC == CRC_read(crc);
        CRC_close(&crc);
    }

    return result;
}


//TODO
LPCLIB_Result IMET_open (IMET_Handle *pHandle)
{
    *pHandle = &_imet;
    IMET_DSP_initAudio();

    return LPCLIB_SUCCESS;
}



/* Send position report */
static void _IMET_sendKiss (IMET_InstanceData *instance)
{
    char s[160];
    char sAltitude[20];
    char sClimbRate[16];
    char sVelocity[8];
    char sDirection[8];
    char *sType;
    int length = 0;
    float f;

    /* Get frequency */
    f = instance->config.frequencyKhz / 1000.0f;

    /* Convert lat/lon from radian to decimal degrees */
    double latitude = instance->gps.observerLLA.lat;
    double longitude = instance->gps.observerLLA.lon;
    if (!isnan(latitude) && !isnan(longitude)) {
        latitude *= 180.0 / M_PI;
        longitude *= 180.0 / M_PI;
    }

    /* Print altitude string first (empty for an invalid altitude) */
    sAltitude[0] = 0;
    if (!isnan(instance->gps.observerLLA.alt)) {
        sprintf(sAltitude, "%.0f", instance->gps.observerLLA.alt);
    }

    /* iMet-1 type indicator */
    sType = "6";

    /* Don't send climb rate if it is undefined */
    sClimbRate[0] = 0;
    if (!isnan(instance->gps.observerLLA.climbRate)) {
        sprintf(sClimbRate, "%.1f", instance->gps.observerLLA.climbRate);
    }

    sVelocity[0] = 0;
    if (!isnan(instance->gps.observerLLA.velocity)) {
        snprintf(sVelocity, sizeof(sVelocity), "%.1f", instance->gps.observerLLA.velocity * 3.6f);
    }
    sDirection[0] = 0;
    if (!isnan(instance->gps.observerLLA.direction)) {
        snprintf(sDirection, sizeof(sDirection), "%.1f", instance->gps.observerLLA.direction * (180.0f / M_PI));
    }

    /* Avoid sending the position if any of the values is undefined */
    if (isnan(latitude) || isnan(longitude)) {
        length = sprintf((char *)s, "%"PRIu32",%s,%.3f,,,,%s,%s,,,,,,,,,%.1f,%.1f,0",
                        instance->id,
                        sType,
                        f,                              /* Frequency [MHz] */
                        sAltitude,                      /* Altitude [m] */
                        sClimbRate,                     /* Climb rate [m/s] */
                        instance->rssi,
                        instance->rxOffset / 1e3f       /* RX frequency offset [kHz] */
                        );
    }
    else {
        length = sprintf((char *)s, "%"PRIu32",%s,%.3f,,%.5lf,%.5lf,%s,%s,%s,%s,%.1f,%.1f,,,%.1f,,%.1f,%.1f,%d,%d,,%.1f,,,%.2lf",
                        instance->id,
                        sType,
                        f,                              /* Frequency [MHz] */
                        latitude,                       /* Latitude [degrees] */
                        longitude,                      /* Longitude [degrees] */
                        sAltitude,                      /* Altitude [m] */
                        sClimbRate,                     /* Climb rate [m/s] */
                        sDirection,
                        sVelocity,
                        instance->metro.temperature,    /* Temperature [°C] */
                        instance->metro.pressure,       /* Pressure [hPa] */
                        instance->metro.humidity,       /* Humidity [%] */
                        instance->rssi,
                        instance->rxOffset / 1e3f,      /* RX frequency offset [kHz] */
                        instance->gps.usedSats,
                        instance->metro.frameCounter,
                        instance->metro.batteryVoltage, /* Battery voltage [V] */
                        instance->realTime * 0.01
                        );
    }

    if (length > 0) {
        SYS_send2Host(HOST_CHANNEL_KISS, s);
    }

    length = sprintf(s, "%"PRIu32",6,0,%s,%.1f,%.1f,%.1f",
                instance->id,
                instance->name,
                instance->metro.temperatureInternal,
                instance->metro.temperaturePSensor,
                instance->metro.temperatureUSensor
                );

    if (length > 0) {
        SYS_send2Host(HOST_CHANNEL_INFO, s);
    }
}



LPCLIB_Result IMET_processBlock (
        IMET_Handle handle,
        void *buffer,
        float rxSetFrequencyHz,
        float rxOffset,
        float rssi,
        uint64_t realTime)
{
    /* Determine length from frame type */
    uint8_t *p = (uint8_t *)buffer;
    int frameType = p[0];
    uint32_t length;
    switch (frameType) {
        case 0x01:
            length = 13;
            break;
        case 0x02:
            length = 17;
            break;
        case 0x03:
            length = p[1] + 4;
            break;
        case 0x04:
            length = 19;
            break;
        case 0x05:
            length = 29;
            break;
        default:
            length = 0;
            break;
    }

    if (length >= 3) {
        if (_IMET_doParityCheck(p, length)) {
            /* Log */
            {
                char log[80];
                unsigned int i;
                snprintf(log, sizeof(log), "%"PRIu32",6,1,",
                            handle->instance->id);
                for (i = 0; i < length; i++) {
                    snprintf(&log[strlen(log)], sizeof(log) - strlen(log), "%02X", p[i]);
                }
                SYS_send2Host(HOST_CHANNEL_INFO, log);
            }

            /* Get/create an instance */
            handle->instance = _IMET_getInstanceDataStructure(rxSetFrequencyHz);
            if (handle->instance) {
                handle->instance->rssi = rssi;
                handle->instance->realTime = realTime;
                switch (frameType) {
                    case IMET_FRAME_GPS:
                        _IMET_processGpsFrame((IMET_FrameGps *)p, &handle->instance->gps);
                        break;
                    case IMET_FRAME_GPSX:
                        _IMET_processGpsxFrame((IMET_FrameGpsx *)p, &handle->instance->gps);
                        break;
                    case IMET_FRAME_PTU:
                        _IMET_processPtuFrame((IMET_FramePtu *)p, &handle->instance->metro);
                        break;
                    case IMET_FRAME_PTUX:
                        _IMET_processPtuxFrame((IMET_FramePtux *)p, &handle->instance->metro);
                        break;
                }

                /* Remember RX (set) frequency and RX offset */
                handle->rxFrequencyHz = rxSetFrequencyHz;
                handle->instance->rxOffset = rxOffset;

                /* If there is a position update, send it out */
                if (handle->instance->gps.updated) {
                    handle->instance->gps.updated = false;

                    _IMET_sendKiss(handle->instance);

                    LPCLIB_Event event;
                    LPCLIB_initEvent(&event, LPCLIB_EVENTID_APPLICATION);
                    event.opcode = APP_EVENT_HEARD_SONDE;
                    event.block = SONDE_DETECTOR_IMET;
                    event.parameter = (void *)((uintptr_t)lrintf(rxSetFrequencyHz));
                    SYS_handleEvent(event);
                }
            }
        }
    }

    return LPCLIB_SUCCESS;
}


/* Send KISS position messages for all known sondes */
LPCLIB_Result IMET_resendLastPositions (IMET_Handle handle)
{
    if (handle == LPCLIB_INVALID_HANDLE) {
        return LPCLIB_ILLEGAL_PARAMETER;
    }

    IMET_InstanceData *instance = NULL;
    while (_IMET_iterateInstance(&instance)) {
        _IMET_sendKiss(instance);
    }

    return LPCLIB_SUCCESS;
}


LPCLIB_Result IMET_pauseResume (IMET_Handle handle, bool pause)
{
    if (handle == LPCLIB_INVALID_HANDLE) {
        return LPCLIB_ILLEGAL_PARAMETER;
    }

    (void)pause;

    /* Simple approach: Reset decode pause begins/ends */
    IMET_DSP_reset();
    
    return LPCLIB_SUCCESS;
}


/* Remove entries from heard list */
LPCLIB_Result IMET_removeFromList (IMET_Handle handle, uint32_t id, float *frequency)
{
    (void)handle;

    IMET_InstanceData *instance = NULL;
    while (_IMET_iterateInstance(&instance)) {
        if (instance->id == id) {
            /* Remove reference from context if this is the current sonde */
            if (instance == handle->instance) {
                handle->instance = NULL;
            }

            /* Let caller know about sonde frequency */
            *frequency = instance->frequency * 1e6f;

            /* Remove sonde */
            _IMET_deleteInstance(instance);
            break;
        }
    }

    return LPCLIB_SUCCESS;
}

