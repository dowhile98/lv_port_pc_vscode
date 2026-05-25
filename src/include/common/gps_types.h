#ifndef GPS_TYPES_H
#define GPS_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "common/date_time.h"

    /**
     * @brief Estado de fix del receptor GPS.
     */
    typedef enum
    {
        GPS_FIX_NONE = 0,
        GPS_FIX_2D = 1,
        GPS_FIX_3D = 2
    } GPSFixStatus_t;

    typedef enum
    {
        GPS_ANTENNA_INTERNAL = 0,
        GPS_ANTENNA_EXTERNAL = 1
    } GPSAntennaType_t;

    /**
     * @brief Última posición GPS conocida.
     */
    typedef struct GPSPosition
    {
        float latitude;
        float longitude;
        float altitude;
        uint8_t satellites_used;
        uint8_t fix_quality;
    } GPSPosition_t;

    typedef struct __attribute__((packed)) GPSConfig
    {
        uint8_t antenna_type;               /**< 0=Internal, 1=External */
        int8_t time_offset;                 /**< Time offset in seconds (-10 to +10) */
        uint8_t utc_offset_index;           /**< Index into UTC_OFFSETS array */
        uint8_t antenna_switch_timeout_min; /**< Auto-switch timeout in minutes (0=disabled, 1-60, default=10) */
    } GPSConfig_t;

    _Static_assert(sizeof(GPSConfig_t) == 4, "GPSConfig_t must be 4 bytes");
    /**
     * @brief Callback disparado en cada PPS válido.
     */
    typedef void (*PPSCallback_t)(void *context, const DateTime_t *time);

#ifdef __cplusplus
}
#endif

#endif /* GPS_TYPES_H */
