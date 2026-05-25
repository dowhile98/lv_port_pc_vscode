/**
 * @file i_general_configuration_alarm_screen_view.h
 * @brief View interface for the General Configuration Alarm selection screen.
 *
 * Implemented by EezGeneralConfigurationAlarmScreenView.
 * Zero lvgl.h dependency — presenters may include this file safely.
 *
 * ## Widget mapping
 *   general_configuration_alarm_list  → lv_list  (ClearList + AddListItem)
 *     Items: "Disable" (id=0), "Enable" (id=1)
 *     Currently-active item receives encoder focus via the `selected` flag.
 *   general_configuration_alarm_label → lv_label (SetLabel — feedback text)
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_GENERAL_CONFIGURATION_ALARM_SCREEN_VIEW_H
#define I_GENERAL_CONFIGURATION_ALARM_SCREEN_VIEW_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ------------------------------------------------------------------ */
    /* Identifiers                                                         */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Opaque item ID passed to the item-selected callback.
     *
     * Matches GeneralConfig_t.buzzer_high_temp_alarm values directly.
     */
    typedef enum
    {
        ALARM_SELECT_DISABLE = 0U, /**< Buzzer high-temp alarm disabled */
        ALARM_SELECT_ENABLE = 1U,  /**< Buzzer high-temp alarm enabled  */
    } AlarmSelectItemId_t;

    /**
     * @brief Callback type registered per list item via AddListItem.
     *
     * @param[in] ctx      Opaque context (presenter pointer).
     * @param[in] item_id  @c AlarmSelectItemId_t value of the selected item.
     */
    typedef void (*AlarmSelectItemCallback_t)(void *ctx, uint8_t item_id);

    /* ------------------------------------------------------------------ */
    /* Interface                                                           */
    /* ------------------------------------------------------------------ */

    typedef struct IGeneralConfigurationAlarmScreenView_t IGeneralConfigurationAlarmScreenView_t;

    struct IGeneralConfigurationAlarmScreenView_t
    {
        /**
         * @brief Remove all items from the selection list.
         *
         * Called before rebuilding via AddListItem.
         */
        void (*ClearList)(IGeneralConfigurationAlarmScreenView_t *self);

        /**
         * @brief Append one item to the selection list.
         *
         * @param[in] self      View instance.
         * @param[in] item_id   @c AlarmSelectItemId_t — passed verbatim to @p cb.
         * @param[in] label     Display string for this item.
         * @param[in] selected  true → set default encoder focus on this item.
         * @param[in] cb        Callback fired when the item is selected.
         * @param[in] ctx       Opaque context forwarded to @p cb.
         */
        void (*AddListItem)(IGeneralConfigurationAlarmScreenView_t *self,
                            uint8_t item_id,
                            const char *label,
                            bool selected,
                            AlarmSelectItemCallback_t cb,
                            void *ctx);

        /**
         * @brief Update the feedback label text.
         *
         * @param[in] self  View instance.
         * @param[in] text  Text to display (NULL → empty string displayed).
         */
        void (*SetLabel)(IGeneralConfigurationAlarmScreenView_t *self, const char *text);
    };

    /* ---------- NULL-safe inline dispatch helpers ---------- */

    static inline void IGeneralConfigurationAlarmScreenView_ClearList(
        IGeneralConfigurationAlarmScreenView_t *v)
    {
        if (v != NULL && v->ClearList != NULL)
        {
            v->ClearList(v);
        }
    }

    static inline void IGeneralConfigurationAlarmScreenView_AddListItem(
        IGeneralConfigurationAlarmScreenView_t *v,
        uint8_t item_id,
        const char *label,
        bool selected,
        AlarmSelectItemCallback_t cb,
        void *ctx)
    {
        if (v != NULL && v->AddListItem != NULL)
        {
            v->AddListItem(v, item_id, label, selected, cb, ctx);
        }
    }

    static inline void IGeneralConfigurationAlarmScreenView_SetLabel(
        IGeneralConfigurationAlarmScreenView_t *v, const char *text)
    {
        if (v != NULL && v->SetLabel != NULL)
        {
            v->SetLabel(v, text);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_GENERAL_CONFIGURATION_ALARM_SCREEN_VIEW_H */
