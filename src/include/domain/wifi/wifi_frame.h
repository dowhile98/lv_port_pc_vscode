/**
 * @file wifi_frame.h
 * @brief Domain types for ESP-Hosted WiFi frame protocol.
 * @version 1.0.0
 * @date 2026-02-16
 * @author Tecna Smart Lab
 *
 * @details
 * Pure domain types for WiFi frame protocol without hardware dependencies.
 * Decouples ESP-Hosted protocol logic from STM32 HAL/CMSIS.
 *
 * @note This is a replacement/abstraction of Third_Party/esp_hosted/common/include/adapter.h
 * @note These types are hardware-agnostic and testable on PC.
 */

#ifndef DOMAIN_WIFI_FRAME_H
#define DOMAIN_WIFI_FRAME_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"

/*============================================================================*
 * CONSTANTS
 *============================================================================*/

/**
 * @brief Maximum WiFi frame payload size (bytes)
 * @note Must match ESP-Hosted protocol:
 *       MAX_PAYLOAD = MAX_SPI_BUFFER_SIZE - WIFI_FRAME_HEADER_SIZE
 *       = 1600 - 12 = 1588 bytes (without debug)
 *       = 1600 - 14 = 1586 bytes (with ESP_PKT_NUM_DEBUG)
 */
#define MAX_SPI_BUFFER_SIZE_TOTAL (1600U)
#if ESP_PKT_NUM_DEBUG
#define WIFI_FRAME_MAX_PAYLOAD_SIZE (MAX_SPI_BUFFER_SIZE_TOTAL - 14U)
#else
#define WIFI_FRAME_MAX_PAYLOAD_SIZE (MAX_SPI_BUFFER_SIZE_TOTAL - 12U)
#endif

/** @brief WiFi frame header size (bytes) */
#if ESP_PKT_NUM_DEBUG
#define WIFI_FRAME_HEADER_SIZE (14U)
#else
#define WIFI_FRAME_HEADER_SIZE (12U)
#endif

/** @brief Maximum total frame size (header + payload) */
#define WIFI_FRAME_MAX_SIZE (WIFI_FRAME_HEADER_SIZE + WIFI_FRAME_MAX_PAYLOAD_SIZE)

/*============================================================================*
 * HEADER FLAGS
 *============================================================================*/

/** @brief Frame has more fragments following */
#define WIFI_FRAME_FLAG_MORE_FRAGMENT (1U << 0)

/** @brief Frame is a wakeup packet */
#define WIFI_FRAME_FLAG_WAKEUP (1U << 1)

/** @brief Power save mode started */
#define WIFI_FRAME_FLAG_POWER_SAVE_ON (1U << 2)

/** @brief Power save mode stopped */
#define WIFI_FRAME_FLAG_POWER_SAVE_OFF (1U << 3)

    /*============================================================================*
     * TYPES
     *============================================================================*/

    /**
     * @brief WiFi interface type.
     *
     * @note Maps to ESP_INTERFACE_TYPE in adapter.h (protocol compatibility).
     */
    typedef enum
    {
        WIFI_IF_STA = 0,    /**< Station mode (client) */
        WIFI_IF_AP = 1,     /**< Access Point mode */
        WIFI_IF_SERIAL = 2, /**< Serial interface (AT commands) */
        WIFI_IF_HCI = 3,    /**< Bluetooth HCI interface */
        WIFI_IF_PRIV = 4,   /**< Private/control interface */
        WIFI_IF_TEST = 5,   /**< Test interface (raw throughput) */
        WIFI_IF_MAX = 6     /**< Sentinel value */
    } WifiInterfaceType_t;

    /**
     * @brief WiFi frame packet type (for private interface).
     *
     * @note Maps to ESP_PRIV_PACKET_TYPE in adapter.h.
     */
    typedef enum
    {
        WIFI_PKT_TYPE_EVENT = 0, /**< Event packet (control path) */
    } WifiPacketType_t;

    /**
     * @brief WiFi frame header (protocol layer).
     *
     * @details
     * This structure matches `struct esp_payload_header` in adapter.h byte-for-byte.
     * It MUST maintain binary compatibility with ESP32 firmware.
     *
     * Layout (12 bytes without debug, 14 bytes with debug):
     * - Byte 0: if_type (4 bits) + if_num (4 bits)
     * - Byte 1: flags
     * - Bytes 2-3: len (uint16_t, little-endian)
     * - Bytes 4-5: offset (uint16_t, little-endian)
     * - Bytes 6-7: checksum (uint16_t, little-endian)
     * - Bytes 8-9: seq_num (uint16_t, little-endian)
     * - Byte 10: reserved2
     * - Bytes 11-12: pkt_num (uint16_t, conditional on ESP_PKT_NUM_DEBUG)
     * - Byte 11/13: hci_pkt_type / priv_pkt_type (union, last byte)
     *
     * @warning DO NOT modify field order or sizes (breaks ESP32 protocol).
     */
    typedef struct __attribute__((packed))
    {
        uint8_t if_type : 4; /**< Interface type (WifiInterfaceType_t) */
        uint8_t if_num : 4;  /**< Interface number (0-15) */
        uint8_t flags;       /**< Frame flags (WIFI_FRAME_FLAG_*) */
        uint16_t len;        /**< Payload length (bytes, NOT including header) */
        uint16_t offset;     /**< Payload offset from frame start (always == WIFI_FRAME_HEADER_SIZE in ESP-Hosted protocol) */
        uint16_t checksum;   /**< Payload checksum (sum of payload bytes) */
        uint16_t seq_num;    /**< Sequence number (for reassembly) */
        uint8_t reserved2;   /**< Reserved byte */

#if ESP_PKT_NUM_DEBUG
        uint16_t pkt_num; /**< Packet number (debug only, for tracking) */
#endif

        union
        {
            uint8_t reserved3;     /**< Generic reserved byte */
            uint8_t hci_pkt_type;  /**< Bluetooth HCI packet type (when if_type == WIFI_IF_HCI) */
            uint8_t priv_pkt_type; /**< Private packet type (when if_type == WIFI_IF_PRIV) */
        };
    } WifiFrameHeader_t;

    /**
     * @brief Complete WiFi frame (header + payload).
     *
     * @note Payload follows header immediately in memory.
     */
    typedef struct
    {
        WifiFrameHeader_t header;                     /**< Frame header (16 bytes) */
        uint8_t payload[WIFI_FRAME_MAX_PAYLOAD_SIZE]; /**< Payload data */
    } WifiFrame_t;

/*============================================================================*
 * COMPILE-TIME ASSERTIONS
 *============================================================================*/

/**
 * @brief Verify header size at compile time.
 *
 * @note Size depends on ESP_PKT_NUM_DEBUG (must match ESP32 firmware):
 *       - 12 bytes when ESP_PKT_NUM_DEBUG=0 (standard mode, production)
 *       - 14 bytes when ESP_PKT_NUM_DEBUG=1 (debug mode, development)
 * @note If this fails, padding was added by compiler (check packing).
 * @warning STM32 and ESP32 firmware MUST be compiled with same ESP_PKT_NUM_DEBUG value!
 */
#if ESP_PKT_NUM_DEBUG
    _Static_assert(sizeof(WifiFrameHeader_t) == 14,
                   "WifiFrameHeader_t size must be exactly 14 bytes with ESP_PKT_NUM_DEBUG=1");
#else
_Static_assert(sizeof(WifiFrameHeader_t) == 12,
               "WifiFrameHeader_t size must be exactly 12 bytes with ESP_PKT_NUM_DEBUG=0");
#endif

    /**
     * @brief Verify interface type enum values match protocol.
     */
    _Static_assert(WIFI_IF_STA == 0 && WIFI_IF_AP == 1 && WIFI_IF_SERIAL == 2,
                   "WifiInterfaceType_t values must match ESP-Hosted protocol");

#ifdef __cplusplus
}
#endif

#endif /* DOMAIN_WIFI_FRAME_H */
