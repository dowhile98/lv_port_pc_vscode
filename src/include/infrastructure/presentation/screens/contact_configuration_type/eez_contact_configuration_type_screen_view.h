/**
 * @file eez_contact_configuration_type_screen_view.h
 * @brief EEZ-backed IContactConfigurationTypeScreenView_t getter.
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_CONTACT_CONFIGURATION_TYPE_SCREEN_VIEW_H
#define EEZ_CONTACT_CONFIGURATION_TYPE_SCREEN_VIEW_H

#include "presentation/interfaces/i_contact_configuration_type_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed view interface.
     *
     * @return Pointer to the statically-allocated view instance.
     */
    IContactConfigurationTypeScreenView_t *EezContactConfigurationTypeScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_CONTACT_CONFIGURATION_TYPE_SCREEN_VIEW_H */
