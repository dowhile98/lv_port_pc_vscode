/**
 * @file eez_information_show_screen_view.c
 * @brief EEZ-backed IInformationShowScreenView_t implementation.
 *
 * Writes to:
 *   objects.information_show_tittle  ← screen title label
 *   objects.information_show_label   ← content label
 *
 * Every lv_* call is guarded by lv_lock() / lv_unlock() — safe to call from
 * any thread context.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
#include "presentation/screens/information_show/eez_information_show_screen_view.h"
#include "ui/screens.h"
#include "lvgl.h"

/* ── vtable implementations ──────────────────────────────────────────────── */

static void set_title(IInformationShowScreenView_t *self, const char *title)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.information_show_tittle, title);
    lv_unlock();
}

static void set_content(IInformationShowScreenView_t *self, const char *content)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.information_show_label, content);
    lv_unlock();
}

/* ── Singleton ───────────────────────────────────────────────────────────── */

static IInformationShowScreenView_t s_vtable = {set_title, set_content};

IInformationShowScreenView_t *EezInformationShowScreenView_GetInterface(void)
{
    return &s_vtable;
}
