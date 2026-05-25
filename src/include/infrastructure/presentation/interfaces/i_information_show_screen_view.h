/**
 * @file i_information_show_screen_view.h
 * @brief View contract for the shared information_show screen.
 *
 * Drives two LVGL labels:
 *   - information_show_tittle  ← screen title (e.g. "Device Status")
 *   - information_show_label   ← multi-line content string
 *
 * @note Zero #include "lvgl.h" — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
#ifndef I_INFORMATION_SHOW_SCREEN_VIEW_H
#define I_INFORMATION_SHOW_SCREEN_VIEW_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief View vtable for the information_show screen.
     */
    typedef struct IInformationShowScreenView_t
    {
        /**
         * @brief Set the screen title label.
         * @param[in] self   View instance.
         * @param[in] title  Null-terminated title string.
         */
        void (*SetTitle)(struct IInformationShowScreenView_t *self, const char *title);

        /**
         * @brief Set the main content label.
         * @param[in] self     View instance.
         * @param[in] content  Null-terminated multi-line content string.
         */
        void (*SetContent)(struct IInformationShowScreenView_t *self, const char *content);
    } IInformationShowScreenView_t;

    /**
     * @brief NULL-safe inline dispatch — SetTitle.
     */
    static inline void IInformationShowScreenView_SetTitle(IInformationShowScreenView_t *self,
                                                           const char *title)
    {
        if (self != NULL && self->SetTitle != NULL)
        {
            self->SetTitle(self, title);
        }
    }

    /**
     * @brief NULL-safe inline dispatch — SetContent.
     */
    static inline void IInformationShowScreenView_SetContent(IInformationShowScreenView_t *self,
                                                             const char *content)
    {
        if (self != NULL && self->SetContent != NULL)
        {
            self->SetContent(self, content);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_INFORMATION_SHOW_SCREEN_VIEW_H */
