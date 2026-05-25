/**
 * @file hourmeter_service.h
 * @brief Pure-domain hourmeter accumulation service (no I/O, no OSAL).
 *
 * @details
 * Encapsulates accumulation logic for 4 hourmeter counters.
 * Operates on RAM data only; persistence is handled by the infrastructure layer.
 *
 * @note  Thread-safety: NOT thread-safe. Caller must synchronize (e.g., via AO).
 */

#ifndef HOURMETER_SERVICE_H
#define HOURMETER_SERVICE_H

#include "common/hourmeter_types.h"
#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── Service Instance ────────────────────────────────────────────────────── */

    /**
     * @brief Hourmeter service (domain logic wrapper around HourmeterData_t).
     */
    typedef struct
    {
        HourmeterData_t data; /**< Accumulator data (RAM cache) */
        bool is_init;         /**< true after Init or LoadFromStorage */
    } Hourmeter_t;

    /* ── Public API ──────────────────────────────────────────────────────────── */

    /**
     * @brief  Initialize hourmeter to default values (all zeros, magic set).
     *
     * @param[out] self  Hourmeter instance.
     * @return ERR_OK on success, ERR_NULL_POINTER if self is NULL.
     */
    Result_t Hourmeter_Init(Hourmeter_t *self);

    /**
     * @brief  Accumulate one minute across all applicable counters.
     *
     * @param[in,out] self          Hourmeter instance.
     * @param[in]     gps_has_fix   true if GPS has 3D fix.
     * @param[in]     relay_active  true if relay is in ON cycle.
     * @param[in]     in_window     true if inside configured time window.
     *
     * @return ERR_OK on success, ERR_NULL_POINTER if self is NULL.
     *
     * @note power_on is always incremented (system is powered).
     */
    Result_t Hourmeter_AccumulateMinute(Hourmeter_t *self,
                                        bool gps_has_fix,
                                        bool relay_active,
                                        bool in_window);

    /**
     * @brief  Get a snapshot of the current hourmeter data.
     *
     * @param[in]  self  Hourmeter instance.
     * @param[out] out   Destination for copy.
     *
     * @return ERR_OK on success, ERR_NULL_POINTER on bad args.
     */
    Result_t Hourmeter_GetData(const Hourmeter_t *self, HourmeterData_t *out);

    /**
     * @brief  Get hours and minutes for a specific accumulator.
     *
     * @param[in]  self     Hourmeter instance.
     * @param[in]  type     Which accumulator.
     * @param[out] hours    Total hours.
     * @param[out] minutes  Additional minutes (0–59).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER or ERR_INVALID_PARAM on error.
     */
    Result_t Hourmeter_GetHours(const Hourmeter_t *self,
                                HourmeterType_t type,
                                uint32_t *hours,
                                uint32_t *minutes);

    /**
     * @brief  Reset all accumulators to zero (factory reset).
     *
     * @param[in,out] self  Hourmeter instance.
     * @return ERR_OK on success, ERR_NULL_POINTER if self is NULL.
     */
    Result_t Hourmeter_Reset(Hourmeter_t *self);

    /**
     * @brief  Load previously persisted data into the service.
     *
     * @param[in,out] self    Hourmeter instance.
     * @param[in]     stored  Data read from EEPROM.
     *
     * @return ERR_OK on success, ERR_NULL_POINTER on bad args.
     */
    Result_t Hourmeter_LoadFromStorage(Hourmeter_t *self, const HourmeterData_t *stored);

    /**
     * @brief  Compute CRC16-CCITT over the data payload of a HourmeterData_t.
     *
     * @param[in] data  Pointer to the data.
     * @return CRC16 value.
     */
    uint16_t Hourmeter_CalculateCRC(const HourmeterData_t *data);

    /**
     * @brief  Update the checksum field in a HourmeterData_t.
     *
     * @param[in,out] data  Data whose checksum to update.
     */
    void Hourmeter_UpdateChecksum(HourmeterData_t *data);

    /**
     * @brief  Validate the CRC of a HourmeterData_t.
     *
     * @param[in] data  Data to validate.
     * @return true if magic matches and CRC is valid.
     */
    bool Hourmeter_ValidateData(const HourmeterData_t *data);

#ifdef __cplusplus
}
#endif

#endif /* HOURMETER_SERVICE_H */
