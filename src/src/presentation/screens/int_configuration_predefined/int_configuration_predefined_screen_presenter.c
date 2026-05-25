/**
 * @file int_configuration_predefined_screen_presenter.c
 * @brief Presenter for the Predefined Cycles selection screen.
 *
 * @note Intentionally does NOT include lvgl.h — all widget access is
 *       delegated to IIntConfigurationPredefinedScreenView_t.
 *
 * ## Flow
 *   OnEnter  → clear state → populate list → show prompt label.
 *   OnSelect → load config → apply ton/toff → save → show "Saved!" → arm timer.
 *   OnUpdate → wait CONFIG_PREDEFINED_SAVED_DISPLAY_MS → navigate back.
 *   OnBack   → navigate back without changes.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/int_configuration_predefined/int_configuration_predefined_screen_presenter.h"
#include "presentation/interfaces/i_screen.h"
#include "interfaces/i_config_storage.h"
#include "common/relay_types.h"
#include <osal/osal.h>
#include <lwprintf/lwprintf.h>
#include <string.h>

/*============================================================================*
 * PRIVATE — predefined cycle table
 *
 * Values in milliseconds.  Order must match EEZ view label table exactly.
 *============================================================================*/

#define PREDEFINED_CYCLE_COUNT 20U

typedef struct
{
    uint32_t ton_ms;  /**< ON  duration (ms). */
    uint32_t toff_ms; /**< OFF duration (ms). */
} PredefinedCycle_t;

static const PredefinedCycle_t s_cycles[PREDEFINED_CYCLE_COUNT] = {
    /* -- Ultra-rápidos --------------------------------------------------- */
    {100U, 100U}, /*  1: 0.1/0.1  s  Rápido simétrico       */
    {200U, 100U}, /*  2: 0.2/0.1  s  Alta velocidad         */
    {300U, 100U}, /*  3: 0.3/0.1  s  CIPS muy rápido        */
    {400U, 100U}, /*  4: 0.4/0.1  s  CIPS computarizado     */
    {300U, 300U}, /*  5: 0.3/0.3  s  DCVG analógico         */
    /* -- Intermedios-rápidos --------------------------------------------- */
    {600U, 200U},  /*  6: 0.6/0.2  s  Caminata dinámica 3:1  */
    {750U, 250U},  /*  7: 0.75/0.25s  3:1 exacto en 1 s      */
    {800U, 200U},  /*  8: 0.8/0.2  s  Gold CIPS 4:1          */
    {900U, 100U},  /*  9: 0.9/0.1  s  Máx tiempo ON          */
    {1200U, 300U}, /* 10: 1.2/0.3  s  4:1 terreno difícil    */
    {1500U, 500U}, /* 11: 1.5/0.5  s  3:1 inductancia        */
    {1600U, 400U}, /* 12: 1.6/0.4  s  4:1 buen recubrimiento */
    /* -- Medios ---------------------------------------------------------- */
    {2000U, 1000U}, /* 13: 2.0/1.0  s  Lectura manual         */
    {2500U, 500U},  /* 14: 2.5/0.5  s  Manual alta energía    */
    {3000U, 1000U}, /* 15: 3.0/1.0  s  3:1 DMM clásico        */
    {4000U, 1000U}, /* 16: 4.0/1.0  s  Estándar universal     */
    {4000U, 2000U}, /* 17: 4.0/2.0  s  Capacitancia alta      */
    /* -- Lentos ---------------------------------------------------------- */
    {8000U, 2000U},  /* 18:  8.0/ 2.0 s Interferencia ductos   */
    {9000U, 1000U},  /* 19:  9.0/ 1.0 s 90% polarización       */
    {12000U, 3000U}, /* 20: 12.0/ 3.0 s Tuberías desnudas      */
};

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

/**
 * @brief Format a millisecond value as "X.XXX" seconds into dest.
 *        dest must be at least 7 bytes.
 */
static void fmt_ms_to_s(uint32_t ms, char *dest, uint32_t dest_size)
{
    uint32_t sec = ms / 1000U;
    uint32_t frac = ms % 1000U;
    (void)lwprintf_snprintf(dest, (size_t)dest_size, "%lu.%03lu", (unsigned long)sec, (unsigned long)frac);
}

/*============================================================================*
 * PRIVATE — selection callback (called by EEZ view on item press)
 *============================================================================*/

static void on_item_selected(uint8_t item_idx, void *context)
{
    IntConfigurationPredefinedScreenPresenter_t *self =
        (IntConfigurationPredefinedScreenPresenter_t *)context;

    if (self == NULL)
    {
        return;
    }
    if (item_idx >= PREDEFINED_CYCLE_COUNT)
    {
        return;
    }

    /* Load → apply → save */
    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
    const Result_t load_res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    if (load_res != ERR_OK)
    {
        IIntConfigurationPredefinedScreenView_SetLabel(self->view, ">> Error loading config");
        return;
    }

    relay_cfg.simple_cycle.ton = s_cycles[item_idx].ton_ms;
    relay_cfg.simple_cycle.toff = s_cycles[item_idx].toff_ms;

    const Result_t save_res = ConfigStorage_SaveRelayConfig(self->config_storage, &relay_cfg);
    if (save_res != ERR_OK)
    {
        IIntConfigurationPredefinedScreenView_SetLabel(self->view, ">> Error saving config");
        return;
    }

    /* Build feedback label: ">> Saved! Ton:X.XXXs Toff:X.XXXs" */
    char ton_str[8];
    char toff_str[8];
    char label_buf[48];
    fmt_ms_to_s(s_cycles[item_idx].ton_ms, ton_str, sizeof(ton_str));
    fmt_ms_to_s(s_cycles[item_idx].toff_ms, toff_str, sizeof(toff_str));

    (void)lwprintf_snprintf(label_buf, sizeof(label_buf),
                            ">> Saved! Ton:%ss Toff:%ss", ton_str, toff_str);

    IIntConfigurationPredefinedScreenView_SetLabel(self->view, label_buf);

    /* Arm auto-return timer. */
    self->show_saved = 1U;
    self->saved_tick = os_ticks_get();
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable callbacks
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    IntConfigurationPredefinedScreenPresenter_t *self =
        (IntConfigurationPredefinedScreenPresenter_t *)base;

    /* Reset save-state from any previous visit. */
    self->show_saved = 0U;
    self->saved_tick = 0U;

    /* Populate list and set prompt. */
    IIntConfigurationPredefinedScreenView_InitItems(
        self->view, on_item_selected, self);

    IIntConfigurationPredefinedScreenView_SetLabel(
        self->view, ">> Predefined Periods");
}

static void screen_on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    IntConfigurationPredefinedScreenPresenter_t *self =
        (IntConfigurationPredefinedScreenPresenter_t *)base;

    if ((self->show_saved != 0U) &&
        ((os_ticks_get() - self->saved_tick) >= CONFIG_PREDEFINED_SAVED_DISPLAY_MS))
    {
        self->show_saved = 0U;
        if (self->router != NULL)
        {
            (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
        }
    }
}

static void on_back_pressed(IScreen_t *base)
{
    IntConfigurationPredefinedScreenPresenter_t *self =
        (IntConfigurationPredefinedScreenPresenter_t *)base;

    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
    }
}

/*============================================================================*
 * PUBLIC — Init
 *============================================================================*/

Result_t IntConfigurationPredefinedScreenPresenter_Init(
    IntConfigurationPredefinedScreenPresenter_t *self,
    const IntConfigurationPredefinedScreenPresenterDeps_t *deps)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->config_storage == NULL)
    {
        return ERR_NULL_POINTER;
    }

    (void)memset(self, 0, sizeof(*self));

    self->base.OnEnter = on_enter;
    self->base.OnExit = screen_on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;

    self->view = deps->view;
    self->config_storage = deps->config_storage;
    self->router = deps->router;
    self->back_screen_id = deps->back_screen_id;

    return ERR_OK;
}
