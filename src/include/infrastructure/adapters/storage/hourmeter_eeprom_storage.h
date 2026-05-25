/**
 * @file hourmeter_eeprom_storage.h
 * @brief Wear-leveled EEPROM persistence for HourmeterData_t.
 *
 * @details
 * Uses 4-slot rotation at EEPROM_HOURMETER_OFFSET (0x0B00).
 * Active slot determined by highest valid write_count.
 * CRC16-CCITT validates data integrity on load.
 */

#ifndef HOURMETER_EEPROM_STORAGE_H
#define HOURMETER_EEPROM_STORAGE_H

#include "common/hourmeter_types.h"
#include "domain/services/hourmeter_service.h"
#include "interfaces/i_ext_eeprom.h"
#include "infrastructure/storage/eeprom_layout.h"
#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── Instance ────────────────────────────────────────────────────────────── */

    /**
     * @brief Hourmeter EEPROM storage with wear-leveling.
     */
    typedef struct
    {
        I_EXT_EEPROM *eeprom;         /**< EEPROM interface (injected) */
        I_EXT_EEPROM_Handle_t handle; /**< EEPROM device handle */
        uint8_t active_slot;          /**< Current active slot (0–3) */
        bool is_init;                 /**< true after Init */
    } HourmeterEepromStorage_t;

    /* ── Public API ──────────────────────────────────────────────────────────── */

    /**
     * @brief  Initialize storage: scan all 4 slots, find newest valid one.
     *
     * @param[out] self    Storage instance.
     * @param[in]  eeprom  EEPROM interface (must not be NULL).
     * @param[in]  handle  EEPROM device handle.
     *
     * @return ERR_OK on success, ERR_NULL_POINTER on bad args.
     */
    Result_t HourmeterEepromStorage_Init(HourmeterEepromStorage_t *self,
                                         I_EXT_EEPROM *eeprom,
                                         I_EXT_EEPROM_Handle_t handle);

    /**
     * @brief  Save hourmeter data to the next wear-leveling slot.
     *
     * Increments write_count, updates CRC, writes to next slot.
     *
     * @param[in,out] self  Storage instance.
     * @param[in,out] data  Data to save (write_count and checksum updated).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER on bad args, ERR_ERROR on I/O fail.
     */
    Result_t HourmeterEepromStorage_Save(HourmeterEepromStorage_t *self,
                                         HourmeterData_t *data);

    /**
     * @brief  Load the most recent valid hourmeter data from EEPROM.
     *
     * Scans all 4 slots, validates CRC, picks highest write_count.
     * Returns defaults if all slots are corrupt.
     *
     * @param[in]  self      Storage instance.
     * @param[out] out_data  Destination for loaded data.
     *
     * @return ERR_OK on success, ERR_NULL_POINTER on bad args.
     */
    Result_t HourmeterEepromStorage_Load(HourmeterEepromStorage_t *self,
                                         HourmeterData_t *out_data);

    /**
     * @brief  Get the slot address for a given slot index.
     *
     * @param[in] slot_index  Slot index (0–3).
     * @return EEPROM address of the slot.
     */
    static inline uint32_t HourmeterEepromStorage_GetSlotAddress(uint8_t slot_index)
    {
        return EEPROM_HOURMETER_OFFSET + ((uint32_t)slot_index * EEPROM_HOURMETER_SLOT_SIZE);
    }

    /**
     * @brief  Get the active slot index from a write_count.
     *
     * @param[in] write_count  Write counter value.
     * @return Slot index (0–3).
     */
    static inline uint8_t HourmeterEepromStorage_GetSlotIndex(uint16_t write_count)
    {
        return write_count % EEPROM_HOURMETER_SLOT_COUNT;
    }

#ifdef __cplusplus
}
#endif

#endif /* HOURMETER_EEPROM_STORAGE_H */
