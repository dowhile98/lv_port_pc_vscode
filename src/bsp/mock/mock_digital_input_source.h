/**
 * @file mock_digital_input_source.h
 * @brief PC mock of IDigitalInputSource — inject events programmatically.
 */
#ifndef MOCK_DIGITAL_INPUT_SOURCE_H
#define MOCK_DIGITAL_INPUT_SOURCE_H

#include "interfaces/i_digital_input_source.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief Return the singleton mock IDigitalInputSource.
     */
    IDigitalInputSource *MockDigitalInputSource_GetInstance(void);

    /**
     * @brief Inject a digital input event (called from keyboard handler or test code).
     *
     * The event is queued and dispatched on the next MockDigitalInputSource_Process() call.
     *
     * @param id    Button/digital input ID.
     * @param event Event type (PRESS, RELEASE, CLICK, KEEPALIVE).
     * @return ERR_OK on success, ERR_BUSY if event queue is full.
     */
    Result_t MockDigitalInputSource_InjectEvent(DigitalInputID_t id, DigitalInputEvent_t event);

    /**
     * @brief Process queued events. Must be called periodically (~20 ms).
     */
    Result_t MockDigitalInputSource_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_DIGITAL_INPUT_SOURCE_H */
