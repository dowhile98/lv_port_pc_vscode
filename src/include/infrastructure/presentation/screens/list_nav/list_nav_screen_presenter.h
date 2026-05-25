/**
 * @file list_nav_screen_presenter.h
 * @brief Generic reusable list-navigation screen presenter.
 *
 * A single presenter implementation shared by all lv_list sub-menu screens
 * (int_configuration, gps_configuration, contact_configuration,
 * general_configuration, and future additions).
 *
 * ## Responsibilities
 *  - OnEnter: populates the lv_list via IListNavScreenView_t::InitItems.
 *  - Item activated: navigates to configured target screen ID (0 = no-op).
 *  - OnBackPressed: navigates to back_screen_id.
 *  - OnUpdate / OnKeyEvent: no-ops — lv_group handles focus.
 *
 * ## Usage
 *  Each screen gets its own static `ListNavScreenPresenter_t` instance.
 *  The injected `view` points to the screen-specific EEZ view implementation
 *  (which targets the correct `objects.*_list` widget).
 *
 * ## Architecture
 *  - IScreen_t is the FIRST field (C99 first-field cast).
 *  - Zero lvgl.h — fully testable on PC.
 *  - Router is optional (NULL → navigation silently skipped in tests).
 *
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#ifndef LIST_NAV_SCREEN_PRESENTER_H
#define LIST_NAV_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_list_nav_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "hal/hal_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Maximum number of list items supported by this presenter. */
#define LIST_NAV_MAX_ITEMS 10U

    /**
     * @brief Dependency bundle for ListNavScreenPresenter_Init().
     */
    typedef struct ListNavScreenPresenterDeps_t
    {
        /** EEZ-backed list view.  Required — must not be NULL. */
        IListNavScreenView_t *view;

        /** Navigation router.  NULL → navigation silently skipped (tests). */
        IScreenRouter_t *router;

        /**
         * @brief Number of items in this screen's list (1 .. LIST_NAV_MAX_ITEMS).
         *
         * Determines how many entries of item_screen_ids[] are valid.
         */
        uint8_t item_count;

        /**
         * @brief Target screen IDs for each list item (0 .. item_count-1).
         *
         * Set item_screen_ids[i] = 0 to mark an entry as "not yet implemented"
         * — the presenter will skip navigation for that index.
         */
        uint8_t item_screen_ids[LIST_NAV_MAX_ITEMS];

        /** Screen to navigate to on OnBackPressed. */
        uint8_t back_screen_id;

        /* -----------------------------------------------------------------------
         * Optional guard hook (backward compatible — NULL = no guard).
         * When guard_fn != NULL, it is called before every item navigation.
         * If it returns false, navigation is denied: on_guard_denied_fn is
         * called (if non-NULL), then the router navigates to
         * guard_denied_screen_id (if != 0).
         * --------------------------------------------------------------------- */

        /**
         * @brief Guard predicate.
         * @param  item_idx  Zero-based list item index.
         * @param  guard_ctx User context (guard_ctx field below).
         * @return true  — allow navigation; false — deny it.
         */
        bool (*guard_fn)(uint8_t item_idx, void *guard_ctx);

        /** Context passed to guard_fn. */
        void *guard_ctx;

        /**
         * @brief Screen ID to navigate to when guard returns false.
         * 0 = silently ignore (do not navigate on denial).
         */
        uint8_t guard_denied_screen_id;

        /**
         * @brief Hook called when guard denies navigation, BEFORE routing to
         * guard_denied_screen_id.  Useful to configure a modal screen.
         * NULL = no-op.
         */
        void (*on_guard_denied_fn)(uint8_t item_idx, void *ctx);

        /** Context passed to on_guard_denied_fn. */
        void *on_guard_denied_ctx;

        /**
         * @brief Hook called when guard passes, BEFORE IScreenRouter_NavigateTo.
         * Useful to set context on the destination screen.
         * NULL = no-op.
         */
        void (*pre_navigate_fn)(uint8_t item_idx, void *ctx);

        /** Context passed to pre_navigate_fn. */
        void *pre_navigate_ctx;

    } ListNavScreenPresenterDeps_t;

    /**
     * @brief Generic list-navigation screen presenter.
     *
     * IScreen_t MUST be the first field — supports C99 first-field cast.
     */
    typedef struct ListNavScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */
        IListNavScreenView_t *view;
        IScreenRouter_t *router;
        uint8_t item_count;
        uint8_t item_screen_ids[LIST_NAV_MAX_ITEMS];
        uint8_t back_screen_id;

        /* Optional guard fields (mirrored from deps) */
        bool (*guard_fn)(uint8_t item_idx, void *guard_ctx);
        void *guard_ctx;
        uint8_t guard_denied_screen_id;
        void (*on_guard_denied_fn)(uint8_t item_idx, void *ctx);
        void *on_guard_denied_ctx;
        void (*pre_navigate_fn)(uint8_t item_idx, void *ctx);
        void *pre_navigate_ctx;
        /**
         * @brief Zero-based index of the last item the user activated.
         *
         * Persists across screen transitions — restored as the focused
         * button when OnEnter() is called again.
         * Initialised to 0 (first item) by ListNavScreenPresenter_Init().
         */
        uint8_t last_selected_idx;
    } ListNavScreenPresenter_t;

    /**
     * @brief Initialise the list-nav screen presenter.
     *
     * @param[in] self  Presenter instance (must not be NULL).
     * @param[in] deps  Dependency bundle (must not be NULL; view must not be NULL;
     *                  item_count must be > 0 and <= LIST_NAV_MAX_ITEMS).
     *
     * @return ERR_OK            on success.
     * @return ERR_NULL_POINTER  if self, deps, or deps->view is NULL.
     * @return ERR_INVALID_PARAM if item_count is 0 or > LIST_NAV_MAX_ITEMS.
     */
    Result_t ListNavScreenPresenter_Init(ListNavScreenPresenter_t *self,
                                         const ListNavScreenPresenterDeps_t *deps);

    /* Public lifecycle delegates — exposed for direct testing; called via vtable in prod */
    void ListNavScreenPresenter_OnEnter(IScreen_t *s);
    void ListNavScreenPresenter_OnExit(IScreen_t *s);
    void ListNavScreenPresenter_OnUpdate(IScreen_t *s);
    void ListNavScreenPresenter_OnBackPressed(IScreen_t *s);

    /**
     * @brief OnKeyEvent — no-op; lv_group manages focus within the list.
     */
    void ListNavScreenPresenter_OnKeyEvent(IScreen_t *s, uint8_t key, uint8_t event);

#ifdef __cplusplus
}
#endif

#endif /* LIST_NAV_SCREEN_PRESENTER_H */
