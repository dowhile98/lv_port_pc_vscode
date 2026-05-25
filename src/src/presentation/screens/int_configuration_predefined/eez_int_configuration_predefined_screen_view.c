/**
 * @file eez_int_configuration_predefined_screen_view.c
 * @brief EEZ-backed IIntConfigurationPredefinedScreenView_t implementation.
 *
 * ONLY file in this screen path allowed to #include "lvgl.h".
 *
 * ## Widget mapping
 *   objects.int_configuration_predefined_list  → lv_list  → InitItems()
 *   objects.int_configuration_predefined_label → lv_label → SetLabel()
 *
 * ## Thread safety
 *   Every lv_* call is wrapped in lv_lock()/lv_unlock() — safe from any thread.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#include "presentation/screens/int_configuration_predefined/eez_int_configuration_predefined_screen_view.h"
#include "presentation/interfaces/i_int_configuration_predefined_screen_view.h"
#include "ui/screens.h"
#include "lvgl.h"
#include <ui/styles.h>
#include <string.h>
#include <stdint.h>

/*============================================================================*
 * PRIVATE — static cycle label table
 *
 * Format: "Ton/Toff s (Total s)"
 * Sorted by total period ascending.
 *============================================================================*/

#define PREDEFINED_CYCLE_LABEL_COUNT 20U

static const char *const s_cycle_labels[PREDEFINED_CYCLE_LABEL_COUNT] = {
    /* ------ Ultra-rápidos (digitales alta velocidad) ---------- */
    " 1. Ton 0.100 / Toff 0.100 s",  /* Rápido simétrico       */
    " 2. Ton 0.200 / Toff 0.100 s",  /* Alta velocidad         */
    " 3. Ton 0.300 / Toff 0.100 s",  /* CIPS muy rápido        */
    " 4. Ton 0.400 / Toff 0.100 s",  /* CIPS computarizado     */
    " 5. Ton 0.300 / Toff 0.300 s",  /* DCVG analógico         */
    /* ------ Intermedios-rápidos (CIPS / Paso a Paso) ---------- */
    " 6. Ton 0.600 / Toff 0.200 s",  /* Caminata dinámica 3:1  */
    " 7. Ton 0.750 / Toff 0.250 s",  /* 3:1 exacto en 1 s      */
    " 8. Ton 0.800 / Toff 0.200 s",  /* Gold CIPS 4:1          */
    " 9. Ton 0.900 / Toff 0.100 s",  /* Máx tiempo ON          */
    "10. Ton 1.200 / Toff 0.300 s",  /* 4:1 terreno difícil    */
    "11. Ton 1.500 / Toff 0.500 s",  /* 3:1 inductancia        */
    "12. Ton 1.600 / Toff 0.400 s",  /* 4:1 buen recubrimiento */
    /* ------ Medios (lecturas manuales / semi-auto) ------------ */
    "13. Ton 2.000 / Toff 1.000 s",  /* Lectura manual         */
    "14. Ton 2.500 / Toff 0.500 s",  /* Manual alta energía ON */
    "15. Ton 3.000 / Toff 1.000 s",  /* 3:1 DMM clásico        */
    "16. Ton 4.000 / Toff 1.000 s",  /* Estándar universal     */
    "17. Ton 4.000 / Toff 2.000 s",  /* Capacitancia alta      */
    /* ------ Lentos (interferencia / despolarización) ---------- */
    "18. Ton  8.000 / Toff  2.000 s", /* Interferencia ductos  */
    "19. Ton  9.000 / Toff  1.000 s", /* 90 % polarización     */
    "20. Ton 12.000 / Toff  3.000 s", /* Tuberías desnudas     */
};

/*============================================================================*
 * PRIVATE — callback trampoline state
 *
 * We need to forward the lv_event to the presenter callback.  A small
 * static context is used because LvglListHelper stores only `void *context`.
 *============================================================================*/

typedef struct
{
    PredefinedCycleSelectedCallback_t on_selected;
    void *context;
} ListCallbackCtx_t;

static ListCallbackCtx_t s_cb_ctx; /* single-screen: no concurrency concern */

/**
 * @brief lv_event_cb forwarding item press → presenter callback.
 */
static void on_item_press(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *list = lv_obj_get_parent(btn);

    /* Find the zero-based index of the pressed button. */
    uint32_t idx = 0U;
    uint32_t child_count = lv_obj_get_child_count(list);
    for (uint32_t i = 0U; i < child_count; i++)
    {
        if (lv_obj_get_child(list, (int32_t)i) == btn)
        {
            idx = i;
            break;
        }
    }

    if ((s_cb_ctx.on_selected != NULL) && (idx < PREDEFINED_CYCLE_LABEL_COUNT))
    {
        s_cb_ctx.on_selected((uint8_t)idx, s_cb_ctx.context);
    }
}

/*============================================================================*
 * PRIVATE — vtable implementations
 *============================================================================*/

/**
 * @brief Clear the list, repopulate with all 20 cycle entries, register callback.
 */
static void init_items(IIntConfigurationPredefinedScreenView_t *self,
                       PredefinedCycleSelectedCallback_t on_selected,
                       void *context)
{
    (void)self;

    s_cb_ctx.on_selected = on_selected;
    s_cb_ctx.context = context;

    lv_lock();

    lv_obj_t *list = objects.int_configuration_predefined_list;
    lv_obj_clean(list);

    for (uint8_t i = 0U; i < PREDEFINED_CYCLE_LABEL_COUNT; i++)
    {
        lv_obj_t *btn = lv_list_add_button(list, NULL, s_cycle_labels[i]);
        add_style_list_button(btn);
        lv_obj_add_event_cb(btn, on_item_press, LV_EVENT_CLICKED, NULL);
    }

    lv_unlock();
}

/**
 * @brief Update the status label.
 */
static void set_label(IIntConfigurationPredefinedScreenView_t *self, const char *text)
{
    (void)self;
    if (text == NULL)
    {
        return;
    }
    lv_lock();
    lv_label_set_text(objects.int_configuration_predefined_label, text);
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static IIntConfigurationPredefinedScreenView_t s_view = {
    .InitItems = init_items,
    .SetLabel  = set_label,
};

/*============================================================================*
 * PUBLIC — getter
 *============================================================================*/

IIntConfigurationPredefinedScreenView_t *
EezIntConfigurationPredefinedScreenView_GetInterface(void)
{
    return &s_view;
}
