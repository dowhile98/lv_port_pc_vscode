/**
 * @file hourmeter_service.c
 * @brief Pure-domain hourmeter accumulation logic (no I/O, no OSAL).
 */

#include "domain/services/hourmeter_service.h"
#include <string.h>

/* ── Private: CRC16-CCITT (poly 0x1021, init 0xFFFF) ────────────────────── */

static uint16_t calculate_crc16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/** @brief Offset of payload data (after header: magic + size + checksum). */
#define HOURMETER_HEADER_SIZE 8U

/**
 * @brief Increment an accumulator pair (hours, minutes) by one minute.
 *        Uses a macro to avoid taking address of packed struct members.
 */
#define ACCUMULATE_PAIR(hours_field, minutes_field)        \
    do                                                     \
    {                                                      \
        (minutes_field)++;                                 \
        if ((minutes_field) >= HOURMETER_MINUTES_PER_HOUR) \
        {                                                  \
            (minutes_field) = 0;                           \
            (hours_field)++;                               \
        }                                                  \
    } while (0)

/* ══════════════════════════════════════════════════════════════════════════ *
 *  Public API                                                               *
 * ══════════════════════════════════════════════════════════════════════════ */

Result_t Hourmeter_Init(Hourmeter_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(Hourmeter_t));
    HourmeterData_InitDefaults(&self->data);
    self->is_init = true;

    return ERR_OK;
}

Result_t Hourmeter_AccumulateMinute(Hourmeter_t *self,
                                    bool gps_has_fix,
                                    bool relay_active,
                                    bool in_window)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    /* Power-on: always */
    ACCUMULATE_PAIR(self->data.power_on_hours, self->data.power_on_minutes);

    /* GPS connected: only when fix */
    if (gps_has_fix)
    {
        ACCUMULATE_PAIR(self->data.gps_connected_hours,
                        self->data.gps_connected_minutes);
    }

    /* Relay active: only when ON */
    if (relay_active)
    {
        ACCUMULATE_PAIR(self->data.relay_active_hours,
                        self->data.relay_active_minutes);
    }

    /* Time window: only when inside */
    if (in_window)
    {
        ACCUMULATE_PAIR(self->data.time_window_hours,
                        self->data.time_window_minutes);
    }

    return ERR_OK;
}

Result_t Hourmeter_GetData(const Hourmeter_t *self, HourmeterData_t *out)
{
    if (!self || !out)
    {
        return ERR_NULL_POINTER;
    }

    memcpy(out, &self->data, sizeof(HourmeterData_t));
    return ERR_OK;
}

Result_t Hourmeter_GetHours(const Hourmeter_t *self,
                            HourmeterType_t type,
                            uint32_t *hours,
                            uint32_t *minutes)
{
    if (!self || !hours || !minutes)
    {
        return ERR_NULL_POINTER;
    }

    switch (type)
    {
    case HOURMETER_TYPE_POWER_ON:
        *hours = self->data.power_on_hours;
        *minutes = self->data.power_on_minutes;
        break;
    case HOURMETER_TYPE_GPS_CONNECTED:
        *hours = self->data.gps_connected_hours;
        *minutes = self->data.gps_connected_minutes;
        break;
    case HOURMETER_TYPE_RELAY_ACTIVE:
        *hours = self->data.relay_active_hours;
        *minutes = self->data.relay_active_minutes;
        break;
    case HOURMETER_TYPE_TIME_WINDOW:
        *hours = self->data.time_window_hours;
        *minutes = self->data.time_window_minutes;
        break;
    default:
        return ERR_INVALID_PARAM;
    }

    return ERR_OK;
}

Result_t Hourmeter_Reset(Hourmeter_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    HourmeterData_InitDefaults(&self->data);
    self->is_init = true;

    return ERR_OK;
}

Result_t Hourmeter_LoadFromStorage(Hourmeter_t *self, const HourmeterData_t *stored)
{
    if (!self || !stored)
    {
        return ERR_NULL_POINTER;
    }

    memcpy(&self->data, stored, sizeof(HourmeterData_t));
    self->is_init = true;

    return ERR_OK;
}

uint16_t Hourmeter_CalculateCRC(const HourmeterData_t *data)
{
    if (!data)
    {
        return 0;
    }

    /* CRC covers everything after the 8-byte header (magic + size + checksum) */
    const uint8_t *payload = (const uint8_t *)data + HOURMETER_HEADER_SIZE;
    uint32_t payload_size = sizeof(HourmeterData_t) - HOURMETER_HEADER_SIZE;

    return calculate_crc16(payload, payload_size);
}

void Hourmeter_UpdateChecksum(HourmeterData_t *data)
{
    if (!data)
    {
        return;
    }
    data->checksum = Hourmeter_CalculateCRC(data);
}

bool Hourmeter_ValidateData(const HourmeterData_t *data)
{
    if (!data)
    {
        return false;
    }
    if (data->magic != HOURMETER_MAGIC)
    {
        return false;
    }

    uint16_t computed = Hourmeter_CalculateCRC(data);
    return (computed == data->checksum);
}
