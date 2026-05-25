/**
 * @file wifi_frame_parser.c
 * @brief WiFi frame parser implementation (domain logic).
 * @version 1.0.0
 * @date 2026-02-16
 * @author Tecna Smart Lab
 *
 * @details
 * Pure domain logic for parsing and validating ESP-Hosted WiFi frames.
 * No hardware dependencies, RTOS dependencies, or logging (testable on PC).
 */

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "domain/wifi/wifi_frame_parser.h"
#include <string.h>

/*============================================================================*
 * PRIVATE FUNCTIONS
 *============================================================================*/

/**
 * @brief Copy header from buffer (handles byte alignment).
 *
 * @param[in] buffer Source buffer.
 * @param[out] header Destination header structure.
 */
static void copy_header_from_buffer(const uint8_t *buffer, WifiFrameHeader_t *header)
{
    /* Use memcpy to avoid alignment issues on ARM Cortex-M */
    memcpy(header, buffer, sizeof(WifiFrameHeader_t));
}

/*============================================================================*
 * PUBLIC API IMPLEMENTATION
 *============================================================================*/

Result_t WifiFrameParser_ParseHeader(const uint8_t *buffer,
                                     uint16_t buffer_len,
                                     WifiFrameHeader_t *out_header)
{
    /* Validate inputs */
    if (buffer == NULL || out_header == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (buffer_len < WIFI_FRAME_HEADER_SIZE)
    {
        return ERR_INVALID_PARAM;
    }

    /* Parse header (little-endian assumed, matches ESP32) */
    copy_header_from_buffer(buffer, out_header);

    return ERR_OK;
}

Result_t WifiFrameParser_ValidateHeader(const WifiFrameHeader_t *header)
{
    if (header == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Validate interface type (must be in range 0-5) */
    if (header->if_type >= WIFI_IF_MAX)
    {
        return ERR_INVALID_PARAM;
    }

    /* Validate payload length (must not exceed maximum) */
    if (header->len > WIFI_FRAME_MAX_PAYLOAD_SIZE)
    {
        return ERR_INVALID_PARAM;
    }

    /* Validate offset field: must equal WIFI_FRAME_HEADER_SIZE per ESP-Hosted protocol.
     * ESP32 always sets offset = sizeof(struct esp_payload_header). */
    if (header->offset != WIFI_FRAME_HEADER_SIZE)
    {
        return ERR_INVALID_PARAM;
    }

    return ERR_OK;
}

uint16_t WifiFrameParser_ComputeChecksum(
    const WifiFrameHeader_t *header,
    const uint8_t *payload,
    uint16_t payload_len)
{
    uint16_t checksum = 0;

    if (header == NULL)
    {
        return 0;
    }

    /* Validate payload pointer if length > 0 */
    if (payload_len > 0 && payload == NULL)
    {
        return 0;
    }

    /* ESP-Hosted protocol: checksum = sum(header except checksum field) + sum(payload) */

    /* Sum header bytes (excluding checksum field at bytes 6-7) */
    const uint8_t *header_bytes = (const uint8_t *)header;
    for (uint32_t i = 0; i < sizeof(WifiFrameHeader_t); i++)
    {
        if (i >= 6 && i < 8)
        {
            continue; /* Skip checksum field itself */
        }
        checksum += header_bytes[i];
    }

    /* Sum payload bytes */
    for (uint16_t i = 0; i < payload_len; i++)
    {
        checksum += payload[i];
    }

    return checksum;
}

Result_t WifiFrameParser_VerifyChecksum(const WifiFrameHeader_t *header,
                                        const uint8_t *payload,
                                        uint16_t payload_len)
{
    if (header == NULL || payload == NULL)
    {
        return ERR_NULL_POINTER;
    }

    (void)payload_len; /* Function verifies header->len bytes — caller-supplied len intentionally ignored */

    /* ESP-Hosted checksum algorithm (matches compute_checksum() in adapter.h):
     * checksum = sum of ALL bytes in (header_with_checksum_zeroed + payload)
     * i.e. sum(rxbuff[0 .. offset + len - 1]) with checksum field = 0 */
    uint16_t computed = 0;

    /* Sum header bytes (checksum field at bytes 6-7 is treated as 0) */
    const uint8_t *header_bytes = (const uint8_t *)header;
    for (uint32_t i = 0; i < sizeof(WifiFrameHeader_t); i++)
    {
        if (i == 6 || i == 7)
            continue; /* Skip checksum field (it was 0 when checksum was computed) */
        computed += header_bytes[i];
    }

    /* Sum payload bytes (header->len bytes — matches what was checksummed at TX) */
    for (uint16_t i = 0; i < header->len; i++)
    {
        computed += payload[i];
    }

    /* Compare against stored checksum */
    if (computed != header->checksum)
    {
        return ERR_CRC_FAIL;
    }

    return ERR_OK;
}

Result_t WifiFrameParser_ParseAndValidate(const uint8_t *buffer,
                                          uint16_t buffer_len,
                                          WifiFrameHeader_t *out_header,
                                          const uint8_t **out_payload_start,
                                          uint16_t *out_payload_len)
{
    /* Validate inputs */
    if (buffer == NULL || out_header == NULL ||
        out_payload_start == NULL || out_payload_len == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Step 1: Parse header */
    Result_t res = WifiFrameParser_ParseHeader(buffer, buffer_len, out_header);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Step 2: Validate header fields */
    res = WifiFrameParser_ValidateHeader(out_header);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Step 3: Check that buffer contains complete frame.
     * offset points to payload start (== WIFI_FRAME_HEADER_SIZE per ESP-Hosted protocol). */
    uint16_t offset = out_header->offset;
    uint32_t required_len = (uint32_t)offset + out_header->len;
    if (buffer_len < required_len)
    {
        return ERR_INVALID_PARAM;
    }

    /* Step 4: Verify checksum.
     * payload starts at buffer + offset. */
    const uint8_t *payload_start = buffer + offset;

    res = WifiFrameParser_VerifyChecksum(out_header, payload_start, out_header->len);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Success: Return payload pointer and length */
    *out_payload_start = payload_start;
    *out_payload_len = out_header->len;

    return ERR_OK;
}

bool WifiFrameParser_HasMoreFragments(const WifiFrameHeader_t *header)
{
    if (header == NULL)
    {
        return false;
    }

    return (header->flags & WIFI_FRAME_FLAG_MORE_FRAGMENT) != 0;
}

bool WifiFrameParser_IsWakeupPacket(const WifiFrameHeader_t *header)
{
    if (header == NULL)
    {
        return false;
    }

    return (header->flags & WIFI_FRAME_FLAG_WAKEUP) != 0;
}

const char *WifiFrameParser_GetInterfaceTypeName(WifiInterfaceType_t if_type)
{
    switch (if_type)
    {
    case WIFI_IF_STA:
        return "STA";
    case WIFI_IF_AP:
        return "AP";
    case WIFI_IF_SERIAL:
        return "SERIAL";
    case WIFI_IF_HCI:
        return "HCI";
    case WIFI_IF_PRIV:
        return "PRIV";
    case WIFI_IF_TEST:
        return "TEST";
    default:
        return "UNKNOWN";
    }
}
