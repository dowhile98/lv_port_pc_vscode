/**
 * @file i_stub_screen_view.h
 * @brief Minimal View interface used to validate the MVP infrastructure end-to-end.
 *
 * This stub is NOT a production screen. Its only purpose is to verify that:
 *   (a) IScreen_t lifecycle methods compile and dispatch correctly, and
 *   (b) the Presenter → View call chain works without any LVGL dependency.
 *
 * Deleted in ACTION-026 once HomeScreenPresenter passes all TDD tests.
 *
 * @note Zero LVGL includes.
 */
#ifndef I_STUB_SCREEN_VIEW_H
#define I_STUB_SCREEN_VIEW_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct IStubScreenView_t IStubScreenView_t;

    struct IStubScreenView_t
    {
        /**
         * @brief Display an arbitrary text message.
         * @param[in] self  View instance.
         * @param[in] msg   Null-terminated message string.
         */
        void (*SetMessage)(IStubScreenView_t *self, const char *msg);
    };

    /** @brief NULL-safe SetMessage dispatch. */
    static inline void IStubScreenView_SetMessage(IStubScreenView_t *v, const char *s)
    {
        if (v && v->SetMessage)
        {
            v->SetMessage(v, s);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_STUB_SCREEN_VIEW_H */
