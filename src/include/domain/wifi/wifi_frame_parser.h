/**
 * @file wifi_frame_parser.h
 * @brief WiFi frame parser and validator (domain logic).
 * @version 1.0.0
 * @date 2026-02-16
 * @author Tecna Smart Lab
 *
 * @details
 * Pure domain logic for parsing and validating ESP-Hosted WiFi frames.
 * No dependencies on hardware, RTOS, or logging (testable on PC).
 *
 * Responsibilities:
 * - Parse raw bytes into WifiFrameHeader_t
 * - Validate header fields (length, checksum, interface type)
 * - Compute and verify payload checksums
 * - Detect protocol errors (malformed frames)
 *
 * @note This replaces inline functions from adapter.h (compute_checksum, etc.)
 */

#ifndef DOMAIN_WIFI_FRAME_PARSER_H
#define DOMAIN_WIFI_FRAME_PARSER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "domain/wifi/wifi_frame.h"
#include "hal_types.h"
#include <stdint.h>
#include <stdbool.h>

    /*============================================================================*
     * PUBLIC API
     *============================================================================*/

    /**
     * @brief Parse WiFi frame header from raw bytes.
     *
     * @details
     * Reads the first 16 bytes of the buffer and populates the header structure.
     * Does NOT validate the header (use WifiFrameParser_ValidateHeader()).
     *
     * @param[in] buffer Raw byte buffer (must be at least 16 bytes).
     * @param[in] buffer_len Buffer length in bytes.
     * @param[out] out_header Parsed header structure (not NULL).
     *
     * @return ERR_OK if parsed successfully.
     * @return ERR_NULL_POINTER if buffer or out_header is NULL.
     * @return ERR_INVALID_PARAM if buffer_len < WIFI_FRAME_HEADER_SIZE.
     *
     * @note Assumes little-endian byte order (matches ESP32).
     * @note Does NOT copy payload, only parses header.
     */
    Result_t WifiFrameParser_ParseHeader(const uint8_t *buffer,
                                         uint16_t buffer_len,
                                         WifiFrameHeader_t *out_header);

    /**
     * @brief Validate WiFi frame header fields.
     *
     * @details
     * Checks header for protocol violations:
     * - Interface type in valid range (0-5)
     * - Payload length <= WIFI_FRAME_MAX_PAYLOAD_SIZE
     * - Offset field == WIFI_FRAME_HEADER_SIZE (ESP-Hosted always sets offset = header size)
     *   offset == 0 means empty/dummy frame with no valid data.
     *
     * @param[in] header Parsed header to validate (not NULL).
     *
     * @return ERR_OK if header is valid.
     * @return ERR_NULL_POINTER if header is NULL.
     * @return ERR_INVALID_PARAM if any field is out of range.
     *
     * @note Does NOT check checksum (use WifiFrameParser_VerifyChecksum()).
     */
    Result_t WifiFrameParser_ValidateHeader(const WifiFrameHeader_t *header);

    /**
     * @brief  Compute checksum over full WiFi frame (header + payload)
     * @note   Implements ESP-Hosted protocol: sum(header except checksum field) + sum(payload)
     *
     * @param[in]  header      Pointer to WiFi frame header (must not be NULL)
     * @param[in]  payload     Pointer to payload data (can be NULL if payload_len == 0)
     * @param[in]  payload_len Length of payload in bytes
     *
     * @return Computed checksum (16-bit sum)
     */
    uint16_t WifiFrameParser_ComputeChecksum(
        const WifiFrameHeader_t *header,
        const uint8_t *payload,
        uint16_t payload_len);

    /**
     * @brief Verify payload checksum against header.
     *
     * @details
     * Computes checksum over header (with checksum field zeroed) + payload bytes,
     * replicating ESP-Hosted compute_checksum(rxbuff, offset+len) algorithm.
     * Uses header->len as payload length (ignores caller-supplied payload_len).
     *
     * @param[in] header  Frame header with expected checksum (not NULL).
     * @param[in] payload Payload data buffer starting at buffer+header->offset (not NULL).
     * @param[in] payload_len Unused (kept for API compatibility). header->len is used.
     *
     * @return ERR_OK if checksum matches.
     * @return ERR_NULL_POINTER if header or payload is NULL.
     * @return ERR_CRC_FAIL if checksum mismatch.
     *
     * @note Algorithm: checksum = sum(header_bytes[i] for i != 6,7) + sum(payload[0..len-1])
     */
    Result_t WifiFrameParser_VerifyChecksum(const WifiFrameHeader_t *header,
                                            const uint8_t *payload,
                                            uint16_t payload_len);

    /**
     * @brief Parse and validate complete frame (header + payload).
     *
     * @details
     * Convenience function combining:
     * 1. WifiFrameParser_ParseHeader()
     * 2. WifiFrameParser_ValidateHeader()
     * 3. WifiFrameParser_VerifyChecksum()
     *
     * @param[in] buffer Raw frame buffer (header + payload, not NULL).
     * @param[in] buffer_len Total buffer length in bytes.
     * @param[out] out_header Parsed and validated header (not NULL).
     * @param[out] out_payload_start Pointer to payload start within buffer (not NULL).
     * @param[out] out_payload_len Actual payload length (not NULL).
     *
     * @return ERR_OK if frame is valid and checksum verified.
     * @return ERR_NULL_POINTER if any pointer is NULL.
     * @return ERR_INVALID_PARAM if buffer_len < (header_size + payload_len).
     * @return ERR_CRC_FAIL if checksum mismatch.
     *
     * @note This is the main entry point for frame parsing.
     * @note out_payload_start points into `buffer` (NOT a copy).
     */
    Result_t WifiFrameParser_ParseAndValidate(const uint8_t *buffer,
                                              uint16_t buffer_len,
                                              WifiFrameHeader_t *out_header,
                                              const uint8_t **out_payload_start,
                                              uint16_t *out_payload_len);

    /**
     * @brief Check if frame has more fragments following.
     *
     * @param[in] header Frame header (not NULL).
     * @return true if MORE_FRAGMENT flag is set, false otherwise.
     */
    bool WifiFrameParser_HasMoreFragments(const WifiFrameHeader_t *header);

    /**
     * @brief Check if frame is a wakeup packet.
     *
     * @param[in] header Frame header (not NULL).
     * @return true if WAKEUP flag is set, false otherwise.
     */
    bool WifiFrameParser_IsWakeupPacket(const WifiFrameHeader_t *header);

    /**
     * @brief Get interface type as string (for debugging/logging).
     *
     * @param[in] if_type Interface type enum value.
     * @return String representation (never NULL, returns "UNKNOWN" for invalid values).
     *
     * @note Safe to call with any uint8_t value.
     */
    const char *WifiFrameParser_GetInterfaceTypeName(WifiInterfaceType_t if_type);

#ifdef __cplusplus
}
#endif

#endif /* DOMAIN_WIFI_FRAME_PARSER_H */
