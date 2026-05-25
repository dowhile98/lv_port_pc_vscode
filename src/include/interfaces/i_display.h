/**
 * @file i_display.h
 * @brief Generic Display Interface - Platform Agnostic (Clean Architecture)
 * @version 1.0.0
 *
 * This header defines the abstract interface for display controllers.
 * Any display driver (ST7789, ILI9341, SSD1306, etc.) can implement this
 * interface, enabling clean separation between GUI libraries (LVGL, U8G2)
 * and hardware-specific code.
 *
 * Design Pattern: Strategy Pattern + V-Table + Dependency Injection
 * This enables Clean Architecture and makes the code highly testable (TDD-friendly).
 *
 * Compatibility:
 *   - LVGL 8.x/9.x (via flush callback with cb_on_complete for DMA)
 *   - U8G2 (via blocking WriteArea calls)
 *
 * @author Tecna Smart Lab
 * @date January 27, 2026
 */

#ifndef INCLUDE_INTERFACES_I_DISPLAY_H_
#define INCLUDE_INTERFACES_I_DISPLAY_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ========================================================================
     * TYPE DEFINITIONS
     * ======================================================================== */

    /** @brief Forward declaration of display interface type */
    typedef struct I_Display I_Display;

    /**
     * @brief Display orientation enumeration.
     *
     * Defines the four possible display orientations. The specific pixel
     * remapping is handled by the concrete driver implementation.
     */
    typedef enum
    {
        I_DISPLAY_ORIENTATION_PORTRAIT = 0,      /**< Portrait (0° rotation) */
        I_DISPLAY_ORIENTATION_LANDSCAPE,         /**< Landscape (90° CW) */
        I_DISPLAY_ORIENTATION_PORTRAIT_INVERTED, /**< Portrait inverted (180°) */
        I_DISPLAY_ORIENTATION_LANDSCAPE_INVERTED /**< Landscape inverted (270°) */
    } Display_Orientation_t;

    /**
     * @brief Display color format enumeration.
     *
     * Defines supported pixel color formats. The concrete driver must
     * configure the display controller to match the selected format.
     */
    typedef enum
    {
        I_DISPLAY_COLOR_FORMAT_MONO = 0, /**< Monochrome (1-bit per pixel) */
        I_DISPLAY_COLOR_FORMAT_RGB565,   /**< 16-bit RGB (5-6-5) - Most common */
        I_DISPLAY_COLOR_FORMAT_RGB666,   /**< 18-bit RGB (6-6-6) - Better quality */
        I_DISPLAY_COLOR_FORMAT_RGB888    /**< 24-bit RGB (8-8-8) - Full color */
    } Display_ColorFormat_t;

    /**
     * @brief Display capabilities structure.
     *
     * Describes the capabilities of a specific display implementation.
     * Queried once during initialization.
     */
    typedef struct
    {
        uint16_t width;                  /**< Display width in pixels */
        uint16_t height;                 /**< Display height in pixels */
        Display_ColorFormat_t color_fmt; /**< Native color format */
        bool supports_dma;               /**< Supports DMA transfers */
        bool supports_backlight;         /**< Has backlight control */
    } Display_Capabilities_t;

    /**
     * @brief Transfer complete callback signature.
     *
     * This callback is invoked when an asynchronous (DMA) transfer completes.
     * It is crucial for LVGL integration where the library must be notified
     * that the framebuffer can be reused.
     *
     * @note This callback is invoked in ISR context. Keep it extremely brief.
     *       Typically, it should only set a flag or signal a semaphore.
     */
    typedef void (*Display_TransferCompleteCallback_t)(void);

    /* ========================================================================
     * V-TABLE DEFINITION (Strategy Pattern)
     * ======================================================================== */

    /**
     * @brief Display Operations Virtual Table (VTable).
     *
     * This structure defines the interface for all display operations. Each
     * concrete implementation (e.g., ST7789, ILI9341, SSD1306) provides its
     * own implementation of these function pointers.
     *
     * The V-Table allows the application (LVGL, U8G2) to work with any display
     * implementation without knowing the underlying hardware details.
     */
    typedef struct I_Display_Vtable
    {
        /**
         * @brief Initialize the display controller.
         *
         * Performs complete hardware initialization including:
         *   - Reset sequence
         *   - Controller configuration (sleep out, pixel format, etc.)
         *   - Default orientation setup
         *
         * @param self Pointer to the display interface instance.
         * @return Result_t
         *   - ERR_OK: Initialization successful
         *   - ERR_NULL_POINTER: Invalid self pointer
         *   - ERR_ERROR: Hardware initialization failed
         */
        Result_t (*Init)(void *self);

        /**
         * @brief Write pixel data to a rectangular area (Async with DMA support).
         *
         * Transfers pixel data to the specified display region. This is the
         * primary method for rendering graphics.
         *
         * For LVGL integration:
         *   - The cb_on_complete callback MUST be called when transfer finishes
         *   - LVGL will wait for this callback before reusing the buffer
         *
         * For U8G2 or blocking usage:
         *   - Pass NULL for cb_on_complete
         *   - Function blocks until transfer completes
         *
         * @param self         Pointer to the display interface instance.
         * @param x1           Left coordinate (inclusive, 0-indexed).
         * @param y1           Top coordinate (inclusive, 0-indexed).
         * @param x2           Right coordinate (inclusive).
         * @param y2           Bottom coordinate (inclusive).
         * @param data         Pointer to pixel buffer (format matches capabilities).
         * @param cb_on_complete Callback invoked when DMA transfer completes.
         *                       Pass NULL for blocking operation.
         * @return Result_t
         *   - ERR_OK: Transfer started successfully (or completed if blocking)
         *   - ERR_NULL_POINTER: Invalid self or data pointer
         *   - ERR_BUSY: Previous transfer still in progress
         *   - ERR_INVALID_PARAM: Coordinates out of bounds
         */
        Result_t (*WriteArea)(void *self,
                              uint16_t x1, uint16_t y1,
                              uint16_t x2, uint16_t y2,
                              uint8_t *data,
                              Display_TransferCompleteCallback_t cb_on_complete);

        /**
         * @brief Set display backlight brightness.
         *
         * @param self    Pointer to the display interface instance.
         * @param percent Brightness level 0-100 (0=off, 100=max).
         * @return Result_t
         *   - ERR_OK: Brightness set successfully
         *   - ERR_NULL_POINTER: Invalid self pointer
         *   - ERR_INVALID_PARAM: percent > 100
         *   - ERR_NOT_SUPPORTED: Backlight control not available
         */
        Result_t (*SetBrightness)(void *self, uint8_t percent);

        /**
         * @brief Set display orientation.
         *
         * Changes the display orientation/rotation. The driver handles
         * the MADCTL register configuration internally.
         *
         * @param self Pointer to the display interface instance.
         * @param ori  Desired orientation.
         * @return Result_t
         *   - ERR_OK: Orientation changed successfully
         *   - ERR_NULL_POINTER: Invalid self pointer
         *   - ERR_INVALID_PARAM: Unknown orientation value
         */
        Result_t (*SetOrientation)(void *self, Display_Orientation_t ori);

        /**
         * @brief Turn display on (exit sleep mode).
         *
         * Wakes the display from sleep mode and enables output.
         *
         * @param self Pointer to the display interface instance.
         * @return Result_t
         *   - ERR_OK: Display turned on successfully
         *   - ERR_NULL_POINTER: Invalid self pointer
         */
        Result_t (*On)(void *self);

        /**
         * @brief Turn display off (enter sleep mode).
         *
         * Puts the display into low-power sleep mode.
         *
         * @param self Pointer to the display interface instance.
         * @return Result_t
         *   - ERR_OK: Display turned off successfully
         *   - ERR_NULL_POINTER: Invalid self pointer
         */
        Result_t (*Off)(void *self);

        /**
         * @brief Get display capabilities.
         *
         * @param self Pointer to the display interface instance.
         * @param caps Pointer to capabilities structure to fill.
         * @return Result_t
         *   - ERR_OK: Capabilities retrieved successfully
         *   - ERR_NULL_POINTER: Invalid self or caps pointer
         */
        Result_t (*GetCapabilities)(void *self, Display_Capabilities_t *caps);

    } I_Display_Vtable;

    /* ========================================================================
     * ABSTRACT OBJECT DEFINITION
     * ======================================================================== */

    /**
     * @brief Display Abstract Interface Object.
     *
     * Base structure that all concrete display implementations must include as
     * the first member to enable polymorphism. The pointer to the VTable (vtable)
     * is initialized by the concrete implementation during initialization.
     *
     * Example usage:
     * @code
     *   ST7789_Driver_t display;
     *   ST7789_Init(&display, spi_iface, gpio_rst, gpio_dc, gpio_bl);
     *   Display_Init(&display.base);
     *   Display_WriteArea(&display.base, 0, 0, 239, 319, framebuffer, lvgl_flush_cb);
     * @endcode
     */
    struct I_Display
    {
        const I_Display_Vtable *vtable;
        void *impl; /**< Pointer to concrete implementation */
    };

    /* ========================================================================
     * HELPER INLINE FUNCTIONS (Polymorphic Dispatch)
     * ======================================================================== */

    static inline bool display_is_valid(const I_Display *iface)
    {
        return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
    }

    /**
     * @brief Initialize the display.
     * @param self Display interface instance.
     * @return Result_t from concrete implementation.
     */
    static inline Result_t Display_Init(I_Display *self)
    {
        if (!display_is_valid(self))
            return ERR_NULL_POINTER;
        return self->vtable->Init(self->impl);
    }

    /**
     * @brief Write pixel data to a rectangular area.
     * @param self         Display interface instance.
     * @param x1           Left coordinate.
     * @param y1           Top coordinate.
     * @param x2           Right coordinate.
     * @param y2           Bottom coordinate.
     * @param data         Pixel buffer.
     * @param cb_on_complete Callback for async completion (NULL for blocking).
     * @return Result_t from concrete implementation.
     */
    static inline Result_t Display_WriteArea(I_Display *self,
                                             uint16_t x1, uint16_t y1,
                                             uint16_t x2, uint16_t y2,
                                             uint8_t *data,
                                             Display_TransferCompleteCallback_t cb_on_complete)
    {
        if (!display_is_valid(self) || data == NULL)
            return ERR_NULL_POINTER;
        return self->vtable->WriteArea(self->impl, x1, y1, x2, y2, data, cb_on_complete);
    }

    /**
     * @brief Set display brightness.
     * @param self    Display interface instance.
     * @param percent Brightness 0-100.
     * @return Result_t from concrete implementation.
     */
    static inline Result_t Display_SetBrightness(I_Display *self, uint8_t percent)
    {
        if (!display_is_valid(self))
            return ERR_NULL_POINTER;
        return self->vtable->SetBrightness(self->impl, percent);
    }

    /**
     * @brief Set display orientation.
     * @param self Display interface instance.
     * @param ori  Desired orientation.
     * @return Result_t from concrete implementation.
     */
    static inline Result_t Display_SetOrientation(I_Display *self, Display_Orientation_t ori)
    {
        if (!display_is_valid(self))
            return ERR_NULL_POINTER;
        return self->vtable->SetOrientation(self->impl, ori);
    }

    /**
     * @brief Turn display on.
     * @param self Display interface instance.
     * @return Result_t from concrete implementation.
     */
    static inline Result_t Display_On(I_Display *self)
    {
        if (!display_is_valid(self))
            return ERR_NULL_POINTER;
        return self->vtable->On(self->impl);
    }

    /**
     * @brief Turn display off.
     * @param self Display interface instance.
     * @return Result_t from concrete implementation.
     */
    static inline Result_t Display_Off(I_Display *self)
    {
        if (!display_is_valid(self))
            return ERR_NULL_POINTER;
        return self->vtable->Off(self->impl);
    }

    /**
     * @brief Get display capabilities.
     * @param self Display interface instance.
     * @param caps Pointer to capabilities structure.
     * @return Result_t from concrete implementation.
     */
    static inline Result_t Display_GetCapabilities(I_Display *self, Display_Capabilities_t *caps)
    {
        if (!display_is_valid(self) || caps == NULL)
            return ERR_NULL_POINTER;
        return self->vtable->GetCapabilities(self->impl, caps);
    }

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_INTERFACES_I_DISPLAY_H_ */
