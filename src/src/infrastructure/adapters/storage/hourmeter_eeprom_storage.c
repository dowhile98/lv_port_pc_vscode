/**
 * @file hourmeter_eeprom_storage.c
 * @brief Wear-leveled EEPROM persistence for hourmeter data (4 slots).
 */

#include "infrastructure/adapters/storage/hourmeter_eeprom_storage.h"
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════ *
 *  Public API                                                               *
 * ══════════════════════════════════════════════════════════════════════════ */

Result_t HourmeterEepromStorage_Init(HourmeterEepromStorage_t *self,
                                     I_EXT_EEPROM *eeprom,
                                     I_EXT_EEPROM_Handle_t handle)
{
    if (!self || !eeprom)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(HourmeterEepromStorage_t));
    self->eeprom = eeprom;
    self->handle = handle;
    self->active_slot = 0;
    self->is_init = true;

    return ERR_OK;
}

Result_t HourmeterEepromStorage_Save(HourmeterEepromStorage_t *self,
                                     HourmeterData_t *data)
{
    if (!self || !data)
    {
        return ERR_NULL_POINTER;
    }

    /* Increment write_count and compute next slot */
    data->write_count++;
    self->active_slot = HourmeterEepromStorage_GetSlotIndex(data->write_count);

    /* Update CRC before writing */
    Hourmeter_UpdateChecksum(data);

    /* Write to EEPROM */
    uint32_t addr = HourmeterEepromStorage_GetSlotAddress(self->active_slot);
    Result_t res = EXT_EEPROM_WriteData(self->eeprom, self->handle,
                                        addr,
                                        (const uint8_t *)data,
                                        sizeof(HourmeterData_t));
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    return ERR_OK;
}

Result_t HourmeterEepromStorage_Load(HourmeterEepromStorage_t *self,
                                     HourmeterData_t *out_data)
{
    if (!self || !out_data)
    {
        return ERR_NULL_POINTER;
    }

    HourmeterData_t candidates[EEPROM_HOURMETER_SLOT_COUNT];
    uint16_t max_write_count = 0;
    int8_t best_slot = -1;

    /* Read all slots */
    for (uint8_t i = 0; i < EEPROM_HOURMETER_SLOT_COUNT; i++)
    {
        uint32_t addr = HourmeterEepromStorage_GetSlotAddress(i);
        Result_t res = EXT_EEPROM_ReadData(self->eeprom, self->handle,
                                           addr,
                                           (uint8_t *)&candidates[i],
                                           sizeof(HourmeterData_t));
        if (res != ERR_OK)
        {
            continue;
        }

        /* Validate magic + CRC */
        if (!Hourmeter_ValidateData(&candidates[i]))
        {
            continue;
        }

        /* Track highest write_count */
        if (best_slot < 0 || candidates[i].write_count > max_write_count)
        {
            max_write_count = candidates[i].write_count;
            best_slot = (int8_t)i;
        }
    }

    if (best_slot >= 0)
    {
        memcpy(out_data, &candidates[best_slot], sizeof(HourmeterData_t));
        self->active_slot = (uint8_t)best_slot;
    }
    else
    {
        /* All slots corrupt — return defaults */
        HourmeterData_InitDefaults(out_data);
        Hourmeter_UpdateChecksum(out_data);
        self->active_slot = 0;
    }

    return ERR_OK;
}
