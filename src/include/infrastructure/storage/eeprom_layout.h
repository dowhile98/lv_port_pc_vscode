/**
 * @file eeprom_layout.h
 * @brief Central EEPROM layout manager — single source of truth for all offsets.
 *
 * Replaces scattered offset defines across config handler headers.
 * Auto-calculates region boundaries using sizeof() and validates at compile time.
 *
 * @note  Device: M24M01E (1 Mbit = 128 KB) or M24M02 (2 Mbit = 256 KB).
 *        Layout v4.3 (gap optimizado: Reserved offset 0x1C000 → 0x1C100).
 *
 * Packed Config Area (auto-cascading offsets):
 *   0x0000  RelayConfig     sizeof(RelayConfigStorage_t)  [216 bytes]
 *   auto    GPSConfig       sizeof(GPSConfigStorage_t)
 *   auto    GeneralConfig   sizeof(GeneralConfigStorage_t)
 *
 * Page-Aligned Auto-Calculated Regions:
 *   auto    WifiConfig      sizeof(WifiConfigStorage_t)    [page-aligned]
 *   auto    SuperUserConfig sizeof(SuperUserConfigStorage_t) [page-aligned]
 *   auto    EventLog        sizeof(EventLogMeta_t) + N * sizeof(EventLogEntry_t)
 *   auto    Hourmeter       4 × sizeof(HourmeterData_t)    [after EventLog]
 *   auto    Reserved        15.75 KB expansion buffer      [gap: 258 bytes]
 */

#ifndef EEPROM_LAYOUT_H
#define EEPROM_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>

/* ── Domain / Common Type Headers (for sizeof auto-calculation) ──────────── */
#include "common/relay_types.h"             /* RelayConfig_t (no TimeWindowConfig_t separate) */
#include "common/gps_types.h"               /* GPSConfig_t                      */
#include "common/general_types.h"           /* GeneralConfig_t                  */
#include "common/wifi_types.h"              /* WifiConfig_t                     */
#include "interfaces/i_config_storage.h"    /* SuperUserConfig_t                */
#include "interfaces/i_event_log_storage.h" /* EventLogMeta_t, EventLogEntry_t  */
#include "common/hourmeter_types.h"         /* HourmeterData_t                  */

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── Device Parameters ───────────────────────────────────────────────────── */

#define EEPROM_TOTAL_SIZE 131072U /**< 128 KB (M24M01E, 1 Mbit) */
#define EEPROM_PAGE_SIZE 256U     /**< Write page size */

    /* ── Layout Version ──────────────────────────────────────────────────────── */

#define EEPROM_LAYOUT_VERSION_MAJOR 4U
#define EEPROM_LAYOUT_VERSION_MINOR 3U /**< v4.3: Reserved offset optimizado (0x1C100), gap 258 bytes */

/* ── Helper Macros ───────────────────────────────────────────────────────── */

/** @brief Calculate end address of a region. */
#define EEPROM_REGION_END(offset, size) ((offset) + (size))

/** @brief Align value up to the next multiple of align_size. */
#define EEPROM_ALIGN_UP(value, align) (((value) + ((align) - 1U)) & ~((align) - 1U))

/* ══════════════════════════════════════════════════════════════════════════ *
 *  Region Sizes — derived from sizeof()                                     *
 *                                                                           *
 *  Config storage wrappers share a common pattern:                          *
 *    magic(4) + size(2) + checksum(2) + data + reserved[N]                  *
 *  The total is sizeof(XxxConfigStorage_t), which we cannot include here    *
 *  (circular dependency). Instead we express sizes as:                      *
 *    EEPROM_CONFIG_HEADER_SIZE + sizeof(DomainType_t) + reserved_N          *
 *  Each handler header _Static_asserts its wrapper matches.                 *
 * ══════════════════════════════════════════════════════════════════════════ */

/** @brief Common storage wrapper overhead: magic(4) + size(2) + checksum(2). */
#define EEPROM_CONFIG_HEADER_SIZE 8U

    /* --- Config Storage Region Sizes (auto-calculated from sizeof) --- */
    /* Reserved bytes generous for RELAY_MAX_CYCLES=10 expansion */

#define EEPROM_RELAY_RESERVED_BYTES 10U
#define EEPROM_RELAY_CONFIG_SIZE (EEPROM_CONFIG_HEADER_SIZE + sizeof(RelayConfig_t) + EEPROM_RELAY_RESERVED_BYTES) /**< 216 = 8 + 198 + 10 */

#define EEPROM_GPS_RESERVED_BYTES 20U                                                                        /**< Must equal sizeof(reserved[]) in GPSConfigStorage_t. */
#define EEPROM_GPS_CONFIG_SIZE (EEPROM_CONFIG_HEADER_SIZE + sizeof(GPSConfig_t) + EEPROM_GPS_RESERVED_BYTES) /**< 32 = 8 + 4 + 20. Capacity: 20 bytes for future fields. */

#define EEPROM_GENERAL_RESERVED_BYTES 15U                                                                                /**< Must equal sizeof(reserved[]) in GeneralConfigStorage_t. */
#define EEPROM_GENERAL_CONFIG_SIZE (EEPROM_CONFIG_HEADER_SIZE + sizeof(GeneralConfig_t) + EEPROM_GENERAL_RESERVED_BYTES) /**< 32 = 8 + 9 + 15. Capacity: 15 bytes for future fields. */

    /* TimeWindow eliminated: now embedded in RelayConfig_t.time_window */

#define EEPROM_WIFI_CONFIG_SIZE (EEPROM_CONFIG_HEADER_SIZE + sizeof(WifiConfig_t)) /**< 256 (exact fit — NO reserved bytes; any WifiConfig_t growth shifts downstream layout) */

/** @note SuperUserConfig_t has its own internal header (magic/size/checksum embedded). */
#define EEPROM_SUPERUSER_RESERVED_BYTES 24U
#define EEPROM_SUPERUSER_CONFIG_SIZE (sizeof(SuperUserConfig_t) + EEPROM_SUPERUSER_RESERVED_BYTES) /**< 64  */

    /* --- Event Log Sizes (no wrapper — written directly) --- */

#define EEPROM_EVENT_LOG_META_SIZE sizeof(EventLogMeta_t)   /**< 6   */
#define EEPROM_EVENT_LOG_ENTRY_SIZE sizeof(EventLogEntry_t) /**< 10  */
#define EEPROM_EVENT_LOG_MAX_ENTRIES 11340U                 /**< Maximized: 113.4 KB / 10 bytes (87.5% EEPROM usage) */
#define EEPROM_EVENT_LOG_ENTRIES_SIZE (EEPROM_EVENT_LOG_ENTRY_SIZE * EEPROM_EVENT_LOG_MAX_ENTRIES)

    /* --- Hourmeter Sizes (self-contained header, no wrapper) --- */

#define EEPROM_HOURMETER_SLOT_SIZE sizeof(HourmeterData_t) /**< 64  */
#define EEPROM_HOURMETER_SLOT_COUNT 4U                     /**< Wear-leveling slots */
#define EEPROM_HOURMETER_TOTAL_SIZE (EEPROM_HOURMETER_SLOT_SIZE * EEPROM_HOURMETER_SLOT_COUNT)

    /* ══════════════════════════════════════════════════════════════════════════ *
     *  Config Area: Auto-Calculated Offsets (packed, cascading)                 *
     *  Layout v4.3: TimeWindow eliminated, page-aligned regions for WiFi+       *
     * ══════════════════════════════════════════════════════════════════════════ */

#define EEPROM_RELAY_CONFIG_OFFSET 0x0000U
#define EEPROM_RELAY_CONFIG_END EEPROM_REGION_END(EEPROM_RELAY_CONFIG_OFFSET, EEPROM_RELAY_CONFIG_SIZE)

#define EEPROM_GPS_CONFIG_OFFSET EEPROM_RELAY_CONFIG_END
#define EEPROM_GPS_CONFIG_END EEPROM_REGION_END(EEPROM_GPS_CONFIG_OFFSET, EEPROM_GPS_CONFIG_SIZE)

#define EEPROM_GENERAL_CONFIG_OFFSET EEPROM_GPS_CONFIG_END
#define EEPROM_GENERAL_CONFIG_END EEPROM_REGION_END(EEPROM_GENERAL_CONFIG_OFFSET, EEPROM_GENERAL_CONFIG_SIZE)

    /* ── Page-Aligned Regions (auto-calculated after configs) ───────────────── */
    /* WiFi, SuperUser, EventLog now auto-align to EEPROM_PAGE_SIZE boundaries   */

#define EEPROM_WIFI_CONFIG_OFFSET EEPROM_ALIGN_UP(EEPROM_GENERAL_CONFIG_END, EEPROM_PAGE_SIZE)
#define EEPROM_WIFI_CONFIG_END EEPROM_REGION_END(EEPROM_WIFI_CONFIG_OFFSET, EEPROM_WIFI_CONFIG_SIZE)

#define EEPROM_SUPERUSER_CONFIG_OFFSET EEPROM_ALIGN_UP(EEPROM_WIFI_CONFIG_END, EEPROM_PAGE_SIZE)
#define EEPROM_SUPERUSER_CONFIG_END EEPROM_REGION_END(EEPROM_SUPERUSER_CONFIG_OFFSET, EEPROM_SUPERUSER_CONFIG_SIZE)

    /* ── Event Log Section (page-aligned after SuperUser) ───────────────────── */
    /* v4.3: 11,340 events × 10 bytes = 113,400 bytes (~110.7 KB, 87.5% EEPROM usage)
     * Gap before Reserved area: 258 bytes (layout v4.3 optimizado, was 2 bytes in v4.2). */

#define EEPROM_EVENT_LOG_OFFSET EEPROM_ALIGN_UP(EEPROM_SUPERUSER_CONFIG_END, EEPROM_PAGE_SIZE)
#define EEPROM_EVENT_LOG_META_OFFSET EEPROM_EVENT_LOG_OFFSET
#define EEPROM_EVENT_LOG_META_END EEPROM_REGION_END(EEPROM_EVENT_LOG_META_OFFSET, EEPROM_EVENT_LOG_META_SIZE)

#define EEPROM_EVENT_LOG_ENTRIES_OFFSET EEPROM_EVENT_LOG_META_END
#define EEPROM_EVENT_LOG_ENTRIES_END EEPROM_REGION_END(EEPROM_EVENT_LOG_ENTRIES_OFFSET, EEPROM_EVENT_LOG_ENTRIES_SIZE)

#define EEPROM_EVENT_LOG_END EEPROM_EVENT_LOG_ENTRIES_END

    /* ── Hourmeter Section (after Event Log, before Reserved) ───────────────── */
    /* 4 slots × 64 bytes = 256 bytes (wear-leveling robusto)
     * Termina en 0x1BFFE (114,686 bytes = 112.00 KB)
     * Gap hasta Reserved: 258 bytes (layout v4.3 optimizado) */

#define EEPROM_HOURMETER_OFFSET EEPROM_EVENT_LOG_END /**< Auto-calculated after event log */
#define EEPROM_HOURMETER_END EEPROM_REGION_END(EEPROM_HOURMETER_OFFSET, EEPROM_HOURMETER_TOTAL_SIZE)

    /* ── Reserved Area (15.75 KB expansion buffer) ──────────────────────────── */
    /* Layout v4.3: Ajustado de 0x1C000 a 0x1C100 para dar 258 bytes de gap
     * antes de Reserved (vs 2 bytes en v4.2.0).
     * Comienza en 0x1C100 (112.25 KB mark = 114,944 bytes)
     * Reservado para futuras expansiones (licencias, logs adicionales, etc.) */

#define EEPROM_RESERVED_OFFSET 0x1C100U                                /**< 112.25 KB mark (layout v4.3 optimizado) */
#define EEPROM_RESERVED_SIZE ((128U * 1024U) - EEPROM_RESERVED_OFFSET) /**< 15.75 KB reserved */
#define EEPROM_RESERVED_END EEPROM_REGION_END(EEPROM_RESERVED_OFFSET, EEPROM_RESERVED_SIZE)

    /* ── Total Used ──────────────────────────────────────────────────────────── */

#define EEPROM_USED_SIZE EEPROM_RESERVED_END /**< Should equal EEPROM_TOTAL_SIZE (100% usage) */

    /* ══════════════════════════════════════════════════════════════════════════ *
     *  Compile-Time Overlap & Capacity Validation                               *
     *  Layout v4.2: Auto-alignment eliminates hardcoded boundary checks         *
     * ══════════════════════════════════════════════════════════════════════════ */

    /* Configs cascade correctly */
    _Static_assert(EEPROM_GPS_CONFIG_OFFSET == EEPROM_RELAY_CONFIG_END,
                   "GPS must follow Relay immediately");

    _Static_assert(EEPROM_GENERAL_CONFIG_OFFSET == EEPROM_GPS_CONFIG_END,
                   "General must follow GPS immediately");

    /* Page-aligned regions don't overlap */
    _Static_assert(EEPROM_WIFI_CONFIG_OFFSET >= EEPROM_GENERAL_CONFIG_END,
                   "WiFi overlaps with config area!");

    _Static_assert(EEPROM_SUPERUSER_CONFIG_OFFSET >= EEPROM_WIFI_CONFIG_END,
                   "SuperUser overlaps with WiFi!");

    _Static_assert(EEPROM_EVENT_LOG_OFFSET >= EEPROM_SUPERUSER_CONFIG_END,
                   "Event Log overlaps with SuperUser!");

    _Static_assert(EEPROM_HOURMETER_OFFSET >= EEPROM_EVENT_LOG_END,
                   "Hourmeter overlaps with Event Log!");

    _Static_assert(EEPROM_HOURMETER_END <= EEPROM_RESERVED_OFFSET,
                   "Hourmeter overlaps with reserved area!");

    /* Verify healthy layout gap (layout v4.3: 258 bytes) */
    _Static_assert((EEPROM_RESERVED_OFFSET - EEPROM_HOURMETER_END) > 128,
                   "CRITICAL: Gap too small! Need at least 128 bytes before Reserved.");

    _Static_assert((EEPROM_RESERVED_OFFSET - EEPROM_HOURMETER_END) < 512,
                   "INFO: Very large gap — consider expanding EventLog/Hourmeter.");

    _Static_assert(EEPROM_RESERVED_END == EEPROM_TOTAL_SIZE,
                   "Reserved area must end exactly at EEPROM boundary!");

    _Static_assert(EEPROM_USED_SIZE == EEPROM_TOTAL_SIZE,
                   "EEPROM layout must use full device capacity!");

    /* Verify RelayConfig_t size matches RELAY_MAX_CYCLES=10 */
    _Static_assert(sizeof(RelayConfig_t) == 198,
                   "RelayConfig_t size mismatch! Expected 198 bytes for RELAY_MAX_CYCLES=10");

    /* ══════════════════════════════════════════════════════════════════════════ *
     *  Region Metadata (runtime introspection & debug)                          *
     * ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Describes a named EEPROM region for runtime queries.
     */
    typedef struct
    {
        const char *name; /**< Human-readable name */
        uint32_t offset;  /**< Start address */
        uint32_t size;    /**< Size in bytes */
        bool is_critical; /**< true = config/data, false = log/diagnostic */
    } EEPROMRegion_t;

/** @brief Number of defined regions in the layout table. */
/** @note v4.2: Reduced from 9 to 8 (TimeWindow removed) */
#define EEPROM_REGION_COUNT 8U

    /** @brief Region table (defined in eeprom_layout.c). */
    extern const EEPROMRegion_t EEPROM_REGION_TABLE[EEPROM_REGION_COUNT];

    /* ══════════════════════════════════════════════════════════════════════════ *
     *  Runtime Helper Functions                                                 *
     * ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Validates that a write at [offset, offset+size) falls within a
     *         defined region and does not cross region boundaries.
     *
     * @param[in] offset  Start address of the write.
     * @param[in] size    Number of bytes to write.
     *
     * @return true if the write is within a single defined region.
     */
    bool EEPROM_Layout_ValidateWriteBounds(uint32_t offset, uint32_t size);

    /**
     * @brief  Returns the region that contains the given offset.
     *
     * @param[in] offset  Address to query.
     *
     * @return Pointer to the EEPROMRegion_t entry, or NULL if offset is in a gap.
     */
    const EEPROMRegion_t *EEPROM_Layout_GetRegionAt(uint32_t offset);

    /**
     * @brief  Returns the total number of defined regions.
     * @return EEPROM_REGION_COUNT.
     */
    uint32_t EEPROM_Layout_GetRegionCount(void);

    /* ══════════════════════════════════════════════════════════════════════════ *
     *  EEPROM Memory Map (Layout v4.3 — Actual Runtime Offsets)              *
     *  Device: M24M01E (128 KB)                                                 *
     * ═════════════════════════════════════════════════════════════════════════ */
    /*
     *  Offset Range         | Region            | Size       | Usage
     *  ---------------------|-------------------|------------|------------------
     *  0x00000 - 0x000D7    | RelayConfig       |  216 bytes | 0.17%
     *  0x000D8 - 0x000F7    | GPSConfig         |   32 bytes | 0.02%
     *  0x000F8 - 0x00117    | GeneralConfig     |   32 bytes | 0.02%
     *  0x00108 - 0x001FF    | <gap>             |  248 bytes | --- (page align)
     *  0x00200 - 0x002FF    | WiFiConfig        |  256 bytes | 0.20% [page-aligned]
     *  0x00300 - 0x0033F    | SuperUserConfig   |   64 bytes | 0.05% [page-aligned]
     *  0x00340 - 0x003FF    | <gap>             |  192 bytes | --- (page align)
     *  0x00400 - 0x1BEFD    | EventLog          |  110.7 KB  | 86.0% [11,340 eventos]
     *    0x00400 - 0x00405  |   ├─ Metadata     |    6 bytes |
     *    0x00406 - 0x1BEFD  |   └─ Entries      |  110.7 KB  | (11,340 × 10 bytes)
     *  0x1BEFE - 0x1BFFD    | Hourmeter         |  256 bytes | 0.20% [4 slots × 64 B]
     *  0x1BFFE - 0x1C0FF    | <gap>             |  258 bytes | ✅ (healthy margin)
     *  0x1C100 - 0x1FFFF    | Reserved          |   15.75 KB | 12.3% [expansión futura]
     *  ---------------------|-------------------|------------|------------------
     *  TOTAL USED (sin reserved):                 112.25 KB    87.7%
     *  TOTAL WITH RESERVED:                       128.0 KB    100.0%
     *
     *  LAYOUT v4.3 IMPROVEMENTS (2026-04-09):
     *  • Gap antes de Reserved: 2 bytes → 258 bytes (129× mejor margen)
     *  • Reserved area: 16.0 KB → 15.75 KB (reducción mínima, sigue generoso)
     *  • Assertions ajustadas: requieren >128 bytes de gap mínimo
     *  • Hourmeter: 4 slots mantenidos para wear-leveling robusto
     *
     *  EXPANSIÓN FUTURA:
     *  Si se necesitan más eventos, incrementar EVENT_LOG_MAX_SIZE hasta agotar
     *  los 258 bytes de gap (~25 eventos adicionales). Para más, reducir Reserved.
     */

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_LAYOUT_H */
