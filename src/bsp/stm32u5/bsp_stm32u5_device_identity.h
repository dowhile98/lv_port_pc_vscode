/**
 * @file bsp_stm32u5_device_identity.h
 * @brief BSP Device Identity — STM32U5 96-bit Unique ID reader.
 *
 * Reads UID96 registers via HAL_GetUIDw0/1/2 and exposes the result through
 * the portable IDeviceIdentity interface.
 *
 * @note Only this BSP file may include stm32u5xx_hal.h for UID access.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
#ifndef BSP_STM32U5_DEVICE_IDENTITY_H
#define BSP_STM32U5_DEVICE_IDENTITY_H

#include "interfaces/i_device_identity.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Concrete BSP implementation of IDeviceIdentity.
     *
     * Statically allocated — call BspDeviceIdentity_Init() once and inject
     * BspDeviceIdentity_GetInterface() into the DI container.
     */
    typedef struct
    {
        IDeviceIdentity iface; /**< Must be first — C99 first-field cast. */
        /* No mutable state: UID is read-only factory ROM. */
    } BspDeviceIdentity_t;

    /**
     * @brief Initialise the BSP device identity instance.
     *
     * @param[in] self  Instance pointer (must not be NULL).
     *
     * @return ERR_OK           on success.
     * @return ERR_NULL_POINTER if self is NULL.
     */
    Result_t BspDeviceIdentity_Init(BspDeviceIdentity_t *self);

    /**
     * @brief Return the IDeviceIdentity interface pointer.
     *
     * @param[in] self  Initialised instance (must not be NULL).
     *
     * @return Pointer to the interface, or NULL if self is NULL.
     */
    IDeviceIdentity *BspDeviceIdentity_GetInterface(BspDeviceIdentity_t *self);

#ifdef __cplusplus
}
#endif

#endif /* BSP_STM32U5_DEVICE_IDENTITY_H */
