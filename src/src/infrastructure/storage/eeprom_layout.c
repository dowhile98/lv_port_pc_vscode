/**
 * @file eeprom_layout.c
 * @brief Runtime helpers for the EEPROM layout manager.
 * @note Layout v4.2: TimeWindow removed (8 regions, was 9).
 */

#include "infrastructure/storage/eeprom_layout.h"
#include <stddef.h>

/* ── Region Table ────────────────────────────────────────────────────────── */
/* v4.2: TimeWindowCfg removed (now embedded in RelayConfig_t) */

const EEPROMRegion_t EEPROM_REGION_TABLE[EEPROM_REGION_COUNT] = {
    {"RelayConfig", EEPROM_RELAY_CONFIG_OFFSET, EEPROM_RELAY_CONFIG_SIZE, true},
    {"GPSConfig", EEPROM_GPS_CONFIG_OFFSET, EEPROM_GPS_CONFIG_SIZE, true},
    {"GeneralConfig", EEPROM_GENERAL_CONFIG_OFFSET, EEPROM_GENERAL_CONFIG_SIZE, true},
    {"WifiConfig", EEPROM_WIFI_CONFIG_OFFSET, EEPROM_WIFI_CONFIG_SIZE, true},
    {"SuperUserConfig", EEPROM_SUPERUSER_CONFIG_OFFSET, EEPROM_SUPERUSER_CONFIG_SIZE, true},
    {"EventLogMeta", EEPROM_EVENT_LOG_META_OFFSET, EEPROM_EVENT_LOG_META_SIZE, false},
    {"EventLogEntries", EEPROM_EVENT_LOG_ENTRIES_OFFSET, EEPROM_EVENT_LOG_ENTRIES_SIZE, false},
    {"Hourmeter", EEPROM_HOURMETER_OFFSET, EEPROM_HOURMETER_TOTAL_SIZE, true},
};

/* ── Runtime Helpers ─────────────────────────────────────────────────────── */

bool EEPROM_Layout_ValidateWriteBounds(uint32_t offset, uint32_t size)
{
    if (size == 0U)
    {
        return false;
    }

    uint32_t end = offset + size;

    /* Overflow check */
    if (end < offset)
    {
        return false;
    }

    for (uint32_t i = 0U; i < EEPROM_REGION_COUNT; i++)
    {
        uint32_t r_start = EEPROM_REGION_TABLE[i].offset;
        uint32_t r_end = r_start + EEPROM_REGION_TABLE[i].size;

        /* Write must be fully contained within a single region */
        if (offset >= r_start && end <= r_end)
        {
            return true;
        }
    }

    return false;
}

const EEPROMRegion_t *EEPROM_Layout_GetRegionAt(uint32_t offset)
{
    for (uint32_t i = 0U; i < EEPROM_REGION_COUNT; i++)
    {
        uint32_t r_start = EEPROM_REGION_TABLE[i].offset;
        uint32_t r_end = r_start + EEPROM_REGION_TABLE[i].size;

        if (offset >= r_start && offset < r_end)
        {
            return &EEPROM_REGION_TABLE[i];
        }
    }

    return NULL;
}

uint32_t EEPROM_Layout_GetRegionCount(void)
{
    return EEPROM_REGION_COUNT;
}
