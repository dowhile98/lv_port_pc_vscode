/**
 * @file ui_actions.h
 * @brief EEZ Studio action implementations + manual event registration.
 *
 * All functions declared/implemented in ui_actions.c run in the LVGL thread
 * context (inside lv_task_handler). They may call lv_*() directly without
 * acquiring lv_lock() — it is already held.
 *
 * ## Screen sequence (ACTION-025)
 *   STARTUP ──[4 000 ms]──► OVERVIEW ──[3 000 ms]──► HOME
 *
 * @note ui_actions_init() MUST be called once from ui_ao.c immediately after
 *       ui_init(), BEFORE the render loop starts.
 *
 * @author Tecna Smart Lab
 * @date   24 de Febrero 2026
 */
#ifndef UI_ACTIONS_H
#define UI_ACTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Register LVGL event callbacks not yet wired by EEZ Studio.
     *
     * Currently registers:
     *   - action_overview_loaded → LV_EVENT_SCREEN_LOAD_START on objects.overview
     *
     * Call once from ui_ao.c immediately after ui_init().
     */
    void ui_actions_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_ACTIONS_H */
