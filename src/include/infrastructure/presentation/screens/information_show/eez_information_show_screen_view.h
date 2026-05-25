/**
 * @file eez_information_show_screen_view.h
 * @brief EEZ-backed IInformationShowScreenView_t singleton getter.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
#ifndef EEZ_INFORMATION_SHOW_SCREEN_VIEW_H
#define EEZ_INFORMATION_SHOW_SCREEN_VIEW_H

#include "presentation/interfaces/i_information_show_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed view for the information_show screen.
     * @return Pointer to the static IInformationShowScreenView_t vtable.
     */
    IInformationShowScreenView_t *EezInformationShowScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_INFORMATION_SHOW_SCREEN_VIEW_H */
