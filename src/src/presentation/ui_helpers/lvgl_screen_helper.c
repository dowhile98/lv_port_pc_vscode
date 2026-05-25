/*
 * lvgl_screen_helper.c
 *
 *  Created on: 11 mar 2026
 *      Author: tecna-smart-lab
 */

#include "presentation/ui_helpers/lvgl_screen_helper.h"

static int16_t currentScreen = -1;

void LvglScreenHelper_Change(enum ScreensEnum screenId)
{
    /*current screen index is screenId - 1 because screenId starts at 1, but our array of screens is zero-indexed. */
    currentScreen = screenId - 1;
    /*verify*/
    if (currentScreen < 0)
    {
        return;
    }

    /*
     * lv_lock() is MANDATORY here: LvglScreenHelper_Change() is called from
     * on_back_pressed() / navigate_to(), which execute in the DigitalInputAO
     * thread (NOT the LVGL task thread).  Without the lock, lv_scr_load_anim()
     * races with lv_task_handler(), corrupting LVGL object trees → HardFault.
     */
    lv_lock();

    /*get the lvgl object corresponding to the current screen index and load it with no animation. */
    lv_obj_t *screen = ((lv_obj_t **)&objects)[currentScreen];
    /*change the currently displayed screen to the new screen with no animation. */
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);

    lv_unlock();
}
