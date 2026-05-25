/**
 * @file i_gps_configuration_antenna_screen_view.h
 * @brief View interface for the GPS Configuration Antenna selection screen.
 *
 * Implemented by EezGpsConfigurationAntennaScreenView. Zero lvgl.h dependency.
 *
 * ## Widget mapping
 *   gps_configuration_antenna_list  → lv_list  (ClearList + AddListItem)
 *     Items: "Internal antenna" (id=0), "External antenna" (id=1)
 *     Currently-active item is visually focused via the `selected` flag.
 *   gps_configuration_antenna_label → lv_label (SetLabel — feedback text)
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_GPS_CONFIGURATION_ANTENNA_SCREEN_VIEW_H
#define I_GPS_CONFIGURATION_ANTENNA_SCREEN_VIEW_H

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
     * Matches GPSConfig_t.antenna_type values directly.
     */
    typedef enum
    {
        ANTENNA_SELECT_INTERNAL = 0U, /**< Internal antenna */
        ANTENNA_SELECT_EXTERNAL = 1U, /**< External antenna */
    } AntennaSelectItemId_t;

    /**
     * @brief Callback type registered per list item via AddListItem.
     *
     * @param[in] ctx      Opaque context (presenter pointer).
     * @param[in] item_id  @c AntennaSelectItemId_t value of the selected item.
     */
    typedef void (*AntennaSelectItemCallback_t)(void *ctx, uint8_t item_id);

    /* ------------------------------------------------------------------ */
    /* Interface                                                           */
    /* ------------------------------------------------------------------ */

    typedef struct IGpsConfigurationAntennaScreenView_t IGpsConfigurationAntennaScreenView_t;

    struct IGpsConfigurationAntennaScreenView_t
    {
        /**
         * @brief Remove all items from the list.
         *
         * Called before rebuilding via AddListItem.
         */
        void (*ClearList)(IGpsConfigurationAntennaScreenView_t *self);

        /**
         * @brief Append one item to the selection list.
         *
         * @param[in] self      View instance.
         * @param[in] item_id   @c AntennaSelectItemId_t — passed verbatim to @p cb.
         * @param[in] label     Display string for this item.
         * @param[in] selected  true → set default encoder focus on this item.
         * @param[in] cb        Callback fired when the item is selected (ENTER/RELEASED).
         * @param[in] ctx       Opaque context forwarded to @p cb.
         */
        void (*AddListItem)(IGpsConfigurationAntennaScreenView_t *self,
                            uint8_t item_id,
                            const char *label,
                            bool selected,
                            AntennaSelectItemCallback_t cb,
                            void *ctx);

        /**
         * @brief Update the feedback label text.
         *
         * @param[in] self  View instance.
         * @param[in] text  Text to display (NULL → empty string displayed).
         */
        void (*SetLabel)(IGpsConfigurationAntennaScreenView_t *self, const char *text);
    };

    /* ---------- NULL-safe inline dispatch helpers ---------- */

    static inline void IGpsConfigurationAntennaScreenView_ClearList(
        IGpsConfigurationAntennaScreenView_t *v)
    {
        if (v != NULL && v->ClearList != NULL)
        {
            v->ClearList(v);
        }
    }

    static inline void IGpsConfigurationAntennaScreenView_AddListItem(
        IGpsConfigurationAntennaScreenView_t *v,
        uint8_t item_id, const char *label, bool selected,
        AntennaSelectItemCallback_t cb, void *ctx)
    {
        if (v != NULL && v->AddListItem != NULL)
        {
            v->AddListItem(v, item_id, label, selected, cb, ctx);
        }
    }

    static inline void IGpsConfigurationAntennaScreenView_SetLabel(
        IGpsConfigurationAntennaScreenView_t *v, const char *text)
    {
        if (v != NULL && v->SetLabel != NULL)
        {
            v->SetLabel(v, text);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_GPS_CONFIGURATION_ANTENNA_SCREEN_VIEW_H */
